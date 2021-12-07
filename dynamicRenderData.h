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



struct unifromContPipe :public pipeline
{
    struct pipelineFuncPtrs
    {
        std::function<void()> sort;
        std::function<void()> internal;
        std::function<void(float)> dt;
    };

    void pipe(float dt)
    {
        
        for(auto it: sort_stage)
            it();
    
        for(auto it:dt_update_stage)
            it(dt);
   
        for(auto it: internal_update_stage)
            it();
    
    }

    std::vector<std::function<void()>> sort_stage;
    std::vector<std::function<void(float)>> dt_update_stage;
    std::vector<std::function<void()>> internal_update_stage;
};


//make common interface with other containers/objects for updateing and stuff
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
    {
        return [this](const float dt){this->pipe(dt);};
    }
    /*
    void addContainer(
        const std::string name,
        uniformContainer* container,
        std::function<void()> sort,
        std::function<void(float)> dt_update,
        std::function<void()> internal_update
    );
    */
    void addContainer(
        const std::string name,
        uniformContainer* container
    );

    void unpackEntryTicket(pipelineFuncPtrs ticket);

    void onSort()
    {
        for(auto it: sort_stage)
            it();
    }
    void onDtUpdate(const float dt)
    {
        for(auto it:dt_update_stage)
            it(dt);
    }
    void onInternalUpdate()
    {
        for(auto it: internal_update_stage)
            it();
    }
    void onGPUUpload()
    {
        for(auto it: on_GPU_upload_stage)
        {
            it();
        }
    }

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
    //unordered map of pipelines anyone??


    std::unordered_map<std::string, uniformContainer*> containers;

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
    uniformContainer( const size_t isize, const int iuID)
    :typeID(iuID), entitySize(isize+isize%8),  
    entityIDs(new int[isize+isize%8]())
    {
        for(size_t i=0;i<entitySize;++i)
        {
            entityIDs[i]=-1;
        }
    }

    //RULE OF 5
    uniformContainer(const uniformContainer& from)
    :IDCounter(from.IDCounter), writeIndex(from.writeIndex), entitySize(from.entitySize)
    {
        entityIDs=new int[entitySize];
        memset(entityIDs, -1, entitySize*sizeof(int));
        memcpy(entityIDs, from.entityIDs, writeIndex*sizeof(int));
    }

    uniformContainer& operator=(const uniformContainer& from)
    {
        if(this==&from) return *this;
        if(from.writeIndex!=this->entitySize)
        {
            this->reAllocate(from.entitySize);
        }

        
        this->IDCounter=from.IDCounter;
        this->entitySize=from.entitySize;
        this->writeIndex=from.writeIndex;
        memcpy(this->entityIDs, from.entityIDs, this->entitySize);
        return *this;
    }
    uniformContainer(uniformContainer&& from)noexcept
    :IDCounter(from.IDCounter), writeIndex(from.writeIndex), entitySize(from.entitySize)
    {
        this->entityIDs=from.entityIDs;
        from.entityIDs=nullptr;
        from.entitySize=0;
    }

    uniformContainer& operator=(uniformContainer&& from)noexcept
    {
        if(this->entityIDs)delete[] this->entityIDs;
        this->IDCounter=from.IDCounter;
        this->entitySize=from.entitySize;
        this->writeIndex=from.writeIndex;
        this->entityIDs=from.entityIDs;
        from.entityIDs=nullptr;
        from.entitySize=0;
        return *this;
    }

    

    //now adds are fast and rmv is slow
    //if you guarantee a sorted list of ints
    //(th sort algo respects the order of ints)
    //you can binary search for ID to remove O(log n)
    int add();
    void remove(const int ID);
    void reAllocate(const size_t newSize)
    {
        int* newArray=new int[newSize+newSize%8];
        memset(newArray, -1, entitySize*sizeof(int));
        memcpy(newArray, entityIDs, writeIndex*sizeof(int));
        delete[] entityIDs;
        entityIDs=allocateAndCopy(newSize, entityIDs, writeIndex);
        entitySize=newSize;
    }




    void sortWithData(std::function<void(int i, int* gather)> subSorts); 
    void onBulkRemoval();

    ~uniformContainer()
    {
        delete[] entityIDs;
    }

    int IDCounter=0;
    int* entityIDs;
    int writeIndex=0;
    std::vector<subBufferHandle> myGPUBuffer;
private:
    std::vector<int> deleteQue;
    int typeID=-1;
    size_t entitySize;
    
public:
    //virutal pipeline entrytickets
    virtual uniformContainer_list::pipelineFuncPtrs makeEntryTicket()=0;
    int end()const{return writeIndex;}
    size_t size()const{return entitySize;}
    int getTypeID()const{return this->typeID;}


    //el printore
    void printIDs(const char* msg="")const
    {
        printf("\n%s", msg);
        for(int i=0; i<writeIndex; ++i)
        {
            printf("\nId: %i, vIndex:%i", entityIDs[i], i);
        }
    }
};



#endif //DYNAMMICRENDERDATA_H