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
struct metaPipeline
{
    
    //make sure all args provide a getPipe() func as seen in the pipeline struct    
    metaPipeline(std::vector<std::string> inames, myargs*... initList )
    :_pipes(std::forward_as_tuple(initList...))
    {
        if(sizeof...(myargs)!=inames.size())
        {
            DEBUGMSG("\n metaPipeline::Ctor=>argcnt doenst match namecnt");
            throw bad_construction_exe(bad_construction_exe::bad_argCnt,
            "\n metaPipeline Construction aborted");
        }
        int i=0;
        for(auto& it: inames)
        {
            names.insert({it, i++});
        }
        onInit<0, myargs...>(_pipes);
    }


    //if your name is not on the list stay THE FUCK away from this
    template<typename T>
    T* getByName(const std::string name)
    {
        int i=names.at(name);
        T* mytee=onSearch<0, T, myargs...>(_pipes, i);
        //assert((std::is_same<T, std::get<i>(_drawables)::type>));
        return mytee;
    } 




    void update(const float dt)const
    {
        for(auto it: onSingleFrame)
        {
            it(dt);
        }
    }

    //could put in init funcs
    std::vector<std::function<void(const float)>> onSingleFrame;
    std::tuple<myargs*...> _pipes;
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
        std::function<void(const float)> f=std::get<I>(t)->getPipe();
        if(f)
            onSingleFrame.push_back(f);
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
typedef metaPipeline<drawables_list, uniformContainer_list> scene;

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
        //_GPUBuffers.insert({name, buf});
        return isGood;
    }

    static gpuBufferTree_head _GPUBuffers;
    //static std::unordered_map<std::string, GPUbuffer> _GPUBuffers;
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
//uploads a mesh into gpu-space and saves it under name in the map
//returns: the vertex count of the uploaded mesh
inline
size_t uploadMesh(const std::string name)
{
    mesh m=meshContainer::getInstance()->getMesh(name);
    if(!m.data)
    {
        throw bad_construction_exe(bad_construction_exe::bad_return,
        "upload of mesh failed: mesh doesnt exist name: "+name);
    }
    subBufferHandle sbh=masterContainer::_GPUBuffers.static_draw.newSubBuffer(m.size);
    if(!sbh.buffer)
    {
        throw bad_construction_exe(bad_construction_exe::bad_gpu_subBufferHandle,
        "upload of mesh failed: bad subBufferHandle name: "+name);
    }
    sbh.buffer->init_subBuffer(sbh.subBufferID, m.perVertexCount, 0, m.data, m.size);
    masterContainer::subBuffers.insert({name, sbh});

    return m.getVertexCount();
}
//uploads a mesh into gpu-space and saves it under name in the map
//returns: the vertex count of the uploaded mesh
inline
size_t uploadMesh(const std::string name, mesh m)
{
    if(!m.data)
    {
        throw bad_construction_exe(bad_construction_exe::bad_return,
        "upload of mesh failed: mesh doesnt exist name: "+name);
    }
    subBufferHandle sbh=masterContainer::_GPUBuffers.static_draw.newSubBuffer(m.size);
    if(!sbh.buffer)
    {
        throw bad_construction_exe(bad_construction_exe::bad_gpu_subBufferHandle,
        "upload of mesh failed: bad subBufferHandle name: "+name);
    }
    sbh.buffer->init_subBuffer(sbh.subBufferID, m.perVertexCount, 0, m.data, m.size);
    masterContainer::subBuffers.insert({name, sbh});

    return m.getVertexCount();
}
inline
void compileShaders()
{
    shaderManager::getInstance()->makeProgram("testShader",solidColorCubeVertexShader, solidColorCubeFragmentShader);
    shaderManager::getInstance()->makeProgram("gridShader",gridShaderWithWave_vertex, solidColorCubeFragmentShader);
}










#endif //MASTERCONTAINER_H