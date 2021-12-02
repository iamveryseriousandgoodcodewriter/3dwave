#ifndef MASTERCONTAINER_H
#define MASTERCONTAINER_H
#include "utils.h"
#include "shader.h"
#include "drawables.h"
//#include <assert.h>
#include <tuple>



//polymorphism does the job better i think
//but this can serve as a contsainer for one up of
//the drawables
//like the base class defines funcs like onInit(), onUpdate()...
//and this takes care of calling those and storing the ptrs
//or like list heads for updating stuff

template<typename ...myargs>
struct drawableContainer
{
    
    
    
    drawableContainer(std::vector<std::string> inames, myargs*... initList )
    :_drawables(std::forward_as_tuple(initList...))
    {
        if(sizeof...(myargs)!=inames.size())
        {
            DEBUGMSG("\n error in drawablecontainer init:argcnt doenst match namecnt");
        }
        int i=0;
        for(auto& it: inames)
        {
            names.insert({it, i++});
        }
        onInit<0, myargs...>(_drawables);
    }


    //if your name is not on the list stay THE FUCK away from this
    template<typename T>
    T* getByName(const std::string name)
    {
        int i=names.at(name);
        T* mytee=onSearch<0, T, myargs...>(_drawables, i);
        //assert((std::is_same<T, std::get<i>(_drawables)::type>));
        return mytee;
    } 




    void update()const
    {

    }

    void draw()
    {
        for(auto it: drawCalls)
        {
            it();
        }
    }


    std::vector<std::function<void()>> drawCalls;
    std::tuple<myargs*...> _drawables;
    std::unordered_map<std::string, int> names;

private:
    template<std::size_t I,typename... args>
    inline typename std::enable_if<I==sizeof...(args), void>::type
    onInit(std::tuple<args*...>& t)
    { }
    template<std::size_t I,typename... args>
    inline typename std::enable_if<I<sizeof...(args), void>::type
    onInit(std::tuple<args*...>& t)
    { 
        std::function<void()> f=std::get<I>(t)->getDrawFunc();
        drawCalls.push_back(f);
        onInit<I+1, args...>(t);
    }
    template<std::size_t I,typename T, typename... args>
    inline typename std::enable_if<I==sizeof...(args), T*>::type
    onSearch(std::tuple<args*...>& t, int i)
    {
        return nullptr;
    }
    
    template<std::size_t I,typename T, typename... args>
    inline typename std::enable_if<I<sizeof...(args), T*>::type
    onSearch(std::tuple<args*...>& t, int i)
    {
        //return std::get<T*>(t);

        if(I<i)
        {
            return onSearch<I+1,T,args...>(t, i);
        }
        else if(I==i)
        {
            return (T*)std::get<I>(t);
        }
        else
        {
            return nullptr;
        }
    }
};

struct subBufferHandle{

    subBufferHandle(const int iID, GPUbuffer* ptr)
    :subBufferID(iID), buffer(ptr){}

    int subBufferID;
    GPUbuffer* buffer;
};

struct masterContainer{


    void free()
    {
        //dont need this yet
    }

    static bool addGPUBuffer(const std::string name, const size_t size, const GLenum drawType)
    {
        GPUbuffer buf(size, GL_FLOAT);
        bool isGood=buf.init(drawType);

        //this looks stupid but we cant add if its not valid???
        if(!isGood)
        {
            return false;
        }
        _GPUBuffers.insert({name, buf});
        return isGood;
    }

    
    static std::unordered_map<std::string, GPUbuffer> _GPUBuffers;
    static std::unordered_map<std::string, drawable*> _drawables;
    static std::unordered_map<std::string, uniformContainer*> _uniformContainers;
    static std::unordered_map<std::string, subBufferHandle> subBuffers;
};
inline
void buildMeshes()
{
    meshContainer::getInstance()->addMesh(allocateMeshPos(), "testVertex");
    meshContainer::getInstance()->addMesh(allocateMeshColor(), "testColor");
    meshContainer::getInstance()->addMesh(allocateSubGridMesh(10), "grid");
}
inline
void compileShaders()
{
    shaderManager::getInstance()->makeProgram("testShader",solidColorCubeVertexShader, solidColorCubeFragmentShader);
    shaderManager::getInstance()->makeProgram("gridShader",gridShaderWithWave_vertex, solidColorCubeFragmentShader);
}
inline
void makeGPUBuffers()
{
    constexpr size_t testCount=1000;
    masterContainer::addGPUBuffer("polygonStatic", meshContainer::getInstance()->getMesh("testColor").size*2, GL_STATIC_DRAW);
    masterContainer::addGPUBuffer("polygonDynamic", testCount*16*2, GL_DYNAMIC_DRAW);

    masterContainer::addGPUBuffer("gridStatic", meshContainer::getInstance()->getMesh("grid").size*2, GL_STATIC_DRAW);
    masterContainer::addGPUBuffer("gridDynamic", 100000*3, GL_DYNAMIC_DRAW);
}


//i guess you could use this now to get a subBuffer, and the rest is done automatically
//up next:
//fill the subbuffer with actual information
//make vaos from subbuffers
//incorporate the uniforms with the drawables and the buffers
//how to organize the subBUffers? having these lookups via the id seems kinda slow and unnessecary, but then again the gpu buffer needs to know about its contents
//also this hides the way things are spread in the buffers making hard to optimize a given draw pipeline
//but i could make a "filled" buffer func for that


struct gpuBufferList;
struct gpuBufferTree_head
{
    gpuBufferList* static_draw;
    gpuBufferList* dynamic_draw;
    gpuBufferList* stream_draw;
};

struct gpuBufferList_node;

//idea: make a list of lists, where each list has an associated size
//opt: sort the list so we know where the full buffers are and dont check them
struct gpuBufferList
{
    subBufferHandle newSubBuffer(const size_t n_size)
    {
        //opt: make it so we dont actually probe the buffers that are too small
        while(next_bufferSize<n_size){next_bufferSize*=2;}
        if(!head)
        {
            return allocateNodeAndAddSubBuffer(head, n_size);
        }
        
        gpuBufferList_node* curr=head;
        int ID=-1;
        while((curr=curr->next)->next)
        {
            if((ID=curr->buffer.newSubBuffer(n_size))>=0)
            {
                //yay :)
                //return the handle?
                return subBufferHandle(ID, &curr->buffer);
                
            }
            
        }
        if((ID=curr->buffer.newSubBuffer(n_size))>=0)
        {
            return subBufferHandle(ID, &curr->buffer);
        }
        if(ID<0)
        {
            while(curr->next){curr=curr->next;}
            return allocateNodeAndAddSubBuffer(curr->next, n_size);
        }

    }

    subBufferHandle allocateNodeAndAddSubBuffer(gpuBufferList_node* ptr, const size_t size)
    {
        int ID=-1;
        try{
                ptr=allocateNewNode(next_bufferSize, type, drawType);
                if((ID=ptr->buffer.newSubBuffer(size))<0)
                {
                    throw 30;
                }
                return subBufferHandle(ID, &ptr->buffer);
            }
            catch(const int& err)
            {
                DEBUGMSG("\n GPUBUFFERLIST::newSubBuffer exe caught, bullshit-buffer returned, msg:");
                switch(err)
                {
                    case 10: DEBUGMSG("\nGPUbuffer bad alloc");break;
                    case 20: DEBUGMSG("\nnew GPU-buffer init failed");break;
                    case 30: DEBUGMSG("\n new subbuffer failed after new allocation, big error here");break;
                    default: DEBUGMSG("\n this should not have happend, call the cops!");break;
                }
            }
        return subBufferHandle(-1, nullptr);
    }
    gpuBufferList_node* allocateNewNode(const size_t size, GLenum type, GLenum drawType)
    {
        gpuBufferList_node* latest=new gpuBufferList_node
        (GPUbuffer(size, type));
        if(!latest)
        {
            throw 10;
        }
        if(!latest->buffer.init(drawType))
        {
            delete latest;
            throw 20;
        }
        return latest;
    }

    //TODO() write this
    void deleteNodes();

    GLenum drawType;
    GLenum type;
    size_t next_bufferSize=1000;
    gpuBufferList_node* head;
};

struct gpuBufferList_node
{
    gpuBufferList_node(const GPUbuffer ibuf)
    :buffer(ibuf){}

    GPUbuffer buffer;
    gpuBufferList_node* next=nullptr;
};











#endif //MASTERCONTAINER_H