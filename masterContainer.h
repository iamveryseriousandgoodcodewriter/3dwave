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














#endif //MASTERCONTAINER_H