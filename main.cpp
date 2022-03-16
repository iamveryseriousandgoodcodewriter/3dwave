#include "context.h"
#include "observer.h"
#include "application.h"




//UTILS STATICS
std::random_device rng::r;
std::mt19937_64 rng::mt{r()};

std::vector<GLuint> UBOwrapper::bindingPoints={0};

//MASTER CONTAINER STATICS
gpuBufferTree GPUmasterContainer::_GPUBuffers;
std::unordered_map<std::string, subBufferHandle> GPUmasterContainer::subBuffers;
struct mainControl : public controlFlags32
{
    enum : int{
        contextOn=1,
        testMain=2,
        testContextMain=4,
        testLoopMain=8
    };
};




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
        //this would maybe need some extensive checking to not call delete on a
        //nullptr
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

    std::vector<int> polygonUniformIDS;
    std::vector<int> subGridIDs;
    


    scene* testScene=initTestScene();
    drawable* grid=populateTestScene(testScene, subGridIDs, polygonUniformIDS);
    


    observer::getInstance()->init(grid);

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
        grid->setUniformFloat(getAlpha(dt, PI), "angle");
        grid->setUniformFloat(observer::getInstance()->spike, "spike");
        grid->setUniformFloat(observer::getInstance()->damp, "damp");


        testScene->update(dt);        
        
        SDL_GL_SwapWindow(context->getCWindow());
    }

    //needs to cleanup now

    clearScene(testScene);
    //call delete on all GPUbuffer_lists in the GPUbufferTree
    GPUmasterContainer::_GPUBuffers.deleteAll();

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