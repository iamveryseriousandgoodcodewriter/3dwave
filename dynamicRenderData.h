#ifndef DYNAMMICRENDERDATA_H
#define DYNAMMICRENDERDATA_H
#include <functional>
#include "utils.h"
#include <string.h>
#include <unordered_map>
#include "math/glm/vec3.hpp"
#include "math/glm/mat4x4.hpp"
#include "math/glm/ext/matrix_transform.hpp"
#include "math/glm/gtx/euler_angles.hpp"
#include "GPUbuffer.h"



//TODOS():
//1make common interface with other containers/objects for updateing and stuff
//2divide the logic into containers, that are containering and pipelines,
//that are using the data
//3make some of these listst(container~pipeline pairs) for different datalayouts
// like big big array, or lisdt of array or whatever
struct uniformContainer;
struct uniformContainer_list : public pipeline
{
    enum pipelinetrafos:int{
        sort=0,
        time_update=1
    };

    //how could one automate this? it is after all very repetitive
    //or generalize this pipeline concept
    struct pipelineFuncPtrs
    {
        std::function<void()> sort;
        std::function<void()> internal;
        std::function<void(float)> dt;
        std::function<void()> GPU_upload;
    };

    std::function<void(const float)> getPipe()
    {return [this](const float dt){this->pipe(dt);};}
    
    void addContainer(const std::string name, uniformContainer* container);
    void deleteAllContainers()
    {
        for(auto& it: containers) delete it.second;
    }

    std::unordered_map<std::string, uniformContainer*> containers;
private:
    void unpackEntryTicket(pipelineFuncPtrs ticket);

    void onSort(){for(auto it: sort_stage)it();}
    void onDtUpdate(const float dt){for(auto it:dt_update_stage)it(dt);}
    void onInternalUpdate(){for(auto it: internal_update_stage)it();}
    void onGPUUpload(){for(auto it: on_GPU_upload_stage)it();}
    void pipe(const float dt)
    {
        this->onSort();
        this->onDtUpdate(dt);
        this->onInternalUpdate();
        this->onGPUUpload();
    }

    //make this its own class "pipline"
    //each pipeline specifies its entry ticket
    //upon entwring this list you say wich pipeline you wanna enter
    std::vector<std::function<void()>> sort_stage;
    std::vector<std::function<void(float)>> dt_update_stage;
    std::vector<std::function<void()>> internal_update_stage;
    std::vector<std::function<void()>> on_GPU_upload_stage;
};
template<typename T>
T* allocateAndCopy(const size_t newSize, T* oldData, const int dataIndex)
{
    T* newArray=new T[newSize+newSize%8];
    memcpy(newArray, oldData, dataIndex*sizeof(T));
    delete[] oldData;
    return newArray;
}


struct uniformContainer{
protected:
    uniformContainer( const size_t isize, const int iuID);
    //RULE OF 5
    uniformContainer(const uniformContainer& from);
    uniformContainer& operator=(const uniformContainer& from);
    uniformContainer(uniformContainer&& from)noexcept;
    uniformContainer& operator=(uniformContainer&& from)noexcept;
    
    

    
        
    int add();

    //removes the ID from the container.
    //actual removal is not happening until the next sort
    void remove(const int ID);
    void reAllocate(const size_t newSize);
    void sortWithData(std::function<void(int, int*)> subSorts); 
    void onBulkRemoval();
    

    int IDCounter=0;
    int* entityIDs;
    int writeIndex=0;
    std::vector<subBufferHandle> myGPUBuffer;
private:
    std::vector<int> deleteQue;
    int typeID=-1;
    size_t entitySize;
    
public:
    struct iterator{
        iterator(const int iindex, uniformContainer* imyCont)
        :index(iindex), myCont(imyCont){}
        
        iterator& operator++(){++index;return *this;}
        iterator& operator--(){--index;return *this;}
        bool operator==(const uniformContainer::iterator& rhs)
        {return this->index==rhs.index;}
        iterator& operator=(const iterator& rhs)=default;
        int operator*(){return *((myCont->entityIDs)+index);}

        int index;
        uniformContainer* myCont;
    };
    iterator begin(){return iterator(0, this);}
    int getTypeID()const{return this->typeID;}
    int end()const{return writeIndex;}
    size_t size()const{return entitySize;}


    //virutal pipeline entrytickets
    virtual uniformContainer_list::pipelineFuncPtrs makeEntryTicket()=0;
    
    virtual ~uniformContainer()
    {
        delete[] entityIDs;
    }

    //el printore
    void printIDs(const char* msg="")const;
};



#endif //DYNAMMICRENDERDATA_H