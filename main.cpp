//#include "context.h"
#include "context.h"
#include "masterContainer.h"
#include "observer.h"
#include "uniform_impl.h"

//UTILS STATICS
std::random_device rng::r;
std::mt19937_64 rng::mt{r()};

std::vector<GLuint> UBOwrapper::bindingPoints={0};

//MASTER CONTAINER STATICS
gpuBufferTree_head masterContainer::_GPUBuffers;
std::unordered_map<std::string, drawable*> masterContainer::_drawables;
std::unordered_map<std::string, uniformContainer*> masterContainer::_uniformContainers;
std::unordered_map<std::string, subBufferHandle> masterContainer::subBuffers;
struct mainControl : public controlFlags32
{
    enum : int{
        contextOn=1,
        testMain=2,
        testContextMain=4,
        testLoopMain=8
    };
};

/*
    1. init gpu buffer
    2. gen some meshes(put in mesh container, save ids)
    3. compile the shaders
    4. set up child class of drawable
    5. draw?
*/


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

void EventMainLoop(bool& quit);
void windowMainLoop(renderContext* context, mainControl& status);
void testWithWindow();
void testMain();

int main(int argc, char** argv)
{

    mainControl status;

    try{
        if(argc>1)
        {
            for(int i=1;i<argc;++i)
            {
                std::string arg=argv[i];
                if(arg=="-w")
                {
                    renderContext::getInstance()->makeWindowAndContext();
                    status.set(mainControl::contextOn);
                }
                else if(arg=="-test")
                {
                    testMain();
                }
                else if(arg=="contextTest")
                {
                    status.set(mainControl::testContextMain);
                }
                else if (arg=="-looptest")
                {
                    status.set(mainControl::testLoopMain);
                }


            }

        }

        if(status.is(mainControl::contextOn))
        {
            if(status.is(mainControl::testContextMain))
            {
                testWithWindow();
            }
            windowMainLoop(renderContext::getInstance(), status);
        }

    }catch(terminal_exe& exe)
    {
        printf("\n terminal exe caught...shutting down");
        switch(exe.ID)
        {
            case terminal_exe::bad_alloc:
                printf("\n cause: bad allocation/corrupted heap, msg: \n%s", exe.msg.c_str());
                break;
            default: printf("\n");break;
        }

        //TODO(): cleanup memory?
    }


    printf("\ndone :)))))\n");
    return 0;
}
void windowMainLoop(renderContext* context, mainControl& status)
{
    bool quit=false;
    const bool test=status.is(mainControl::testLoopMain);
    timer T;
    T.start();
    float dt;
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    //camera is still really weird
    //the polygons arent above the origins, but they should be    
    
    buildMeshes();
    compileShaders();
    //makeGPUBuffers(); one could inititalzie some of theses upfront?

    uniformContainer_list dynamic_data_list;
    size_t vertCnt1, vertCnt2;
    //TODO(): wrap this up in a func
    try{
        dynamic_data_list.addContainer(
            "uniform3d_polygonDynamics",
            new uniform3d(1000, "uniform3d_polygonDynamics")
        );

        //these dont really belong here cause they are very much static but well...
        dynamic_data_list.addContainer(
            "gridUniforms_",
            new gridUniforms(10000, {"grid_trans", "grid_scale"})
        );
        vertCnt1=uploadMesh("testVertex");
        uploadMesh("testColor");
        vertCnt2=uploadMesh("grid");

    }catch(bad_construction_exe& exe)
    {
        DEBUGMSG("\n bad construction_exe caught on uniform creation");
        switch(exe.ID)
        {
            case bad_construction_exe::bad_gpu_subBufferHandle:
                DEBUGMSG("\n"+exe.msg);
                break;
            case bad_construction_exe::bad_alloc:
                DEBUGMSG("\n"+exe.msg);
                throw terminal_exe(terminal_exe::bad_alloc, "");
                break;
            default: break;
        }
    }

    drawable polygon(shaderManager::getInstance()->getShader("testShader"));
    polygon.init(vertCnt1,
        {
        oneVertexArraySetup(masterContainer::subBuffers.at("testVertex"), 0, 0),
        oneVertexArraySetup(masterContainer::subBuffers.at("testColor"), 0, 1),
        oneVertexArraySetup(masterContainer::subBuffers.at("uniform3d_polygonDynamics"), 1, 2)
        }
    );

    drawable grid(shaderManager::getInstance()->getShader("gridShader"));
    grid.init(vertCnt2,
        {
            oneVertexArraySetup(masterContainer::subBuffers.at("grid"), 0, 0),
            oneVertexArraySetup(masterContainer::subBuffers.at("grid_trans"), 1, 1),
            oneVertexArraySetup(masterContainer::subBuffers.at("grid_scale"), 1, 2),
        }
    );
    grid.drawMode=GL_POINTS;

    
    
    std::vector<int> ploygonUniformIDS;
    std::vector<int> subGridIDs;
    

    gridUniforms* lol=(gridUniforms*)dynamic_data_list.containers.at("gridUniforms_");
    subGridIDs=lol->makeDiagonalSquare(2.0f);
    grid.instanceCount=lol->end();
    lol->onGPUUpload();

        
    float* data=genOrigins(10, 20.0f, PI/20);

    uniform3d* ref=(uniform3d*)dynamic_data_list.containers.at("uniform3d_polygonDynamics");

    auto adderall=[](const std::vector<glm::vec3> pos){
        std::vector<oneUniform3d> ret;
        for(auto it: pos)
        {
            ret.push_back(oneUniform3d(it, glm::vec3(0.0f), glm::vec3(2.0f)));
        }  
        return ret;
    };
    ploygonUniformIDS=ref->addN(adderall(makevec3(data, 10)));
    polygon.instanceCount=ref->end();

    UBOwrapper origins("waveOrigins", 10, 10*4);
    origins.init(GL_STATIC_DRAW);
    origins.updateUBO(data);
    free(data);


    observer::getInstance()->init();

    int i=0;
    while(!quit)
    {
        EventMainLoop(quit);
        dt=T.getDeltaDur();
        observer::getInstance()->update(dt);
        //printf("\nenter the main deltadur:%f, fps:%f", dt, (float)1/dt);

        glClearColor(0.0f, 0.0f, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        if(test)
        {
        }
        grid.setUniformFloat(getAlpha(dt, PI), "angle");
        grid.setUniformFloat(observer::getInstance()->spike, "spike");
        grid.setUniformFloat(observer::getInstance()->damp, "damp");

        dynamic_data_list.pipe();
        polygon.draw();
        grid.draw();
        
        SDL_GL_SwapWindow(context->getCWindow());
    }
}

void EventMainLoop(bool& quit)
{
    SDL_Event e;
    while(SDL_PollEvent(&e))
    {
        if(e.type==SDL_KEYDOWN)
        {
            if(e.key.keysym.sym==SDLK_ESCAPE)
            {
                quit=true;
            }
        }
        observer::getInstance()->input(e);
    }

}

//called with and active render context
void testWithWindow()
{

}

    //called wiht and activeRender Context and inside the main loop
void testInMainLoop()
{

}


//called without a window
void testMain()
{
    
}