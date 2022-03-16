#ifndef APPLICATION_H
#define APPLICATION_H
#include "masterContainer.h"
#include "uniform_impl.h"

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
    subBufferHandle sbh=GPUmasterContainer::_GPUBuffers.static_draw.newSubBuffer(m.size);
    if(!sbh.buffer)
    {
        throw bad_construction_exe(bad_construction_exe::bad_gpu_subBufferHandle,
        "upload of mesh failed: bad subBufferHandle name: "+name);
    }
    sbh.buffer->init_subBuffer(sbh.subBufferID, m.perVertexCount, 0, m.data, m.size);
    GPUmasterContainer::subBuffers.insert({name, sbh});

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
    subBufferHandle sbh=GPUmasterContainer::_GPUBuffers.static_draw.newSubBuffer(m.size);
    if(!sbh.buffer)
    {
        throw bad_construction_exe(bad_construction_exe::bad_gpu_subBufferHandle,
        "upload of mesh failed: bad subBufferHandle name: "+name);
    }
    sbh.buffer->init_subBuffer(sbh.subBufferID, m.perVertexCount, 0, m.data, m.size);
    GPUmasterContainer::subBuffers.insert({name, sbh});

    return m.getVertexCount();
}
inline
void compileShaders()
{
    shaderManager::getInstance()->makeProgram("testShader",solidColorCubeVertexShader, solidColorCubeFragmentShader);
    shaderManager::getInstance()->makeProgram("gridShader",gridShaderWithWave_vertex, solidColorCubeFragmentShader);
}

float getAlpha(const float dt, const float w)
{
    static float alpha=0;
    alpha+=w*dt;

    if(alpha>2*PI)
    {
        alpha=0;
    }

    return alpha;
}

float* genOrigins(int count, float r, float dphi)
{
    float dang=2*PI/count;
    float* data=(float*)malloc(count*4*sizeof(float));
    float* dp=data;
    for(int i=0;i<count;++i)
    {
        *dp++=r*cos(dang*i);
        *dp++=r*sin(dang*i);
        *dp++=dphi*i+1.1234567f;
        *dp++=0;
    }
    dp=data;
    
    return data;
}

std::vector<glm::vec3> makevec3(float* data, int count)
{
    std::vector<glm::vec3> ret;
    for(int i=0; i<count;++i, data+=4)
    {
        glm::vec3 v(*data, *(data+1), 10.0f);
        ret.push_back(v);
    }
    return ret;
}

drawable* constructDrawable(
    const std::string name, 
    const GLuint shader, 
    const int max_cnt, 
    const uniformID uniformContainer,
    const std::vector<std::string> meshNames, 
    scene& _scene
);
scene* initTestScene()
{
    
    scene* myScene=nullptr;
    try{
        myScene=new scene(
            {"drawables", "uniform_Containers"},
            new drawables_list,
            new uniformContainer_list
        );
        
        constructDrawable(
            "grid",
            shaderManager::getInstance()->programs.at("gridShader"),
            10000, uniformID::gridUniform,
            {"grid"}, *myScene
        );
        constructDrawable(
            "polygon",
            shaderManager::getInstance()->programs.at("testShader"),
            100, uniformID::_uniform3d,
            {"testVertex", "testColor"}, *myScene
        );
    }
    catch(bad_construction_exe& exe)
    {
        DEBUGMSG("\n "+exe.msg);
        throw terminal_exe(terminal_exe::bad_scene_construction,
        "cant init metaPipeline");
    }
    return myScene;
}

//this wants to be a little bit more automated:
//  constructing the uniformContainers needs to happen more elegent
//meshNames need to be given in the oreder of their shader location
drawable* constructDrawable(
    const std::string name, 
    const GLuint shader, 
    const int max_cnt, 
    const uniformID uniformContainerID,
    const std::vector<std::string> meshNames, 
    scene& _scene)
{
    uniformContainer_list* ul=_scene.getByName<uniformContainer_list>("uniform_Containers");
    drawables_list* dl=_scene.getByName<drawables_list>("drawables");
    drawable& d=dl->add(name, drawable(shader));
    size_t vCnt;//hack!!!CALL THE CODE POLICE!!!!
    int i=0;
    for(auto& it: meshNames)
    {
        vCnt=uploadMesh(it);
        d.vertexArraySetup(GPUmasterContainer::subBuffers.at(it), 0, i++);
    }

    if(uniformContainerID==uniformID::gridUniform)
    {
        gridUniforms* gcont=new gridUniforms(max_cnt,name);
        ul->addContainer(name+"_Dynamics", gcont);
        d.init(vCnt, gcont, gcont->getVAOstats(name));
    }
    else if(uniformContainerID==uniformID::_uniform3d)
    {
        uniform3d* cont=new uniform3d(max_cnt, name);
        ul->addContainer(name+"_Dynamics", cont);
        d.init(vCnt, cont, cont->getVAOstats(name));
    }
    else
    {
        DEBUGMSG("\n constructDrawable called with bad uniformID");
        throw bad_construction_exe(bad_construction_exe::bad_arg,
        "drawableContsruction aborted");
    }

    return &d;
}

drawable* populateTestScene(scene* testScene, std::vector<int>& v1, std::vector<int>&v2)
{
    float* data=genOrigins(10, 50.0f, PI/20);
        auto adderall=[](const std::vector<glm::vec3> pos){
        std::vector<oneUniform3d> ret;
        for(auto it: pos)
        {
            ret.push_back(oneUniform3d(it, glm::vec3(0.0f), glm::vec3(2.0f)));
        }  
        return ret;
    };

    drawable* grid=&testScene->getByName<drawables_list>("drawables")->drawables.at("grid");
    grid->drawMode=GL_LINE_LOOP;
    v1=grid->addNEntity<gridUniforms>(
        makeDiagonalGridSquare(2.0f, grid->myEntities->size()-grid->myEntities->end())
    );

    drawable* d=&testScene->getByName<drawables_list>("drawables")->drawables.at("polygon");    
    v2=d->addNEntity<uniform3d>(adderall(makevec3(data, 10)));


    UBOwrapper origins("waveOrigins", 10, 10*4);
    origins.init(GL_STATIC_DRAW);
    origins.updateUBO(data);
    free(data);
    return grid;
}


void clearScene(scene* _scene)
{
    //delete the arrays in the uniform_impl
    //delete the uniformContainer* in the maps inside the "lists"
    _scene->getByName<uniformContainer_list>("uniform_Containers")->deleteAllContainers();
    
    //call delete on the *_list pointers held by the scene object
    delete _scene->getByName<uniformContainer_list>("uniform_Container");
    delete _scene->getByName<drawables_list>("drawables");
    delete _scene;
}


#endif //APPLICATION_H