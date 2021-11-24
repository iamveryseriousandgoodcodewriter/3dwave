//#include "context.h"
#include "context.h"
#include "masterContainer.h"
#include "observer.h"

//UTILS STATICS
std::random_device rng::r;
std::mt19937_64 rng::mt{r()};

std::vector<GLuint> UBOwrapper::bindingPoints={0};

//MASTER CONTAINER STATICS
std::unordered_map<std::string, GPUbuffer> masterContainer::_GPUBuffers;
std::unordered_map<std::string, drawable*> masterContainer::_drawables;
std::unordered_map<std::string, uniformContainer*> masterContainer::_uniformContainers;
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
    makeGPUBuffers();
  

    drawableContainer<solidColorPolygon, gridDrawer>*
    drawer=new drawableContainer<solidColorPolygon, gridDrawer>(
        std::vector<std::string>{"polygons", "grids"},
        new solidColorPolygon(
            1000,
            &masterContainer::_GPUBuffers.at("polygonStatic"),
            &masterContainer::_GPUBuffers.at("polygonDynamic"),
            shaderManager::getInstance()->getShader("testShader"),
            std::vector<std::string>{"testVertex", "testColor"}
        ),
        new gridDrawer(
            10000,
            &masterContainer::_GPUBuffers.at("gridStatic"),
            &masterContainer::_GPUBuffers.at("gridDynamic"),
            shaderManager::getInstance()->getShader("gridShader"),
            std::vector<std::string>{"grid"}
        )
    );
    
    std::vector<int> ploygonUniformIDS;
    std::vector<int> subGridIDs;
        
        
    float* data=genOrigins(10, 20.0f, PI/20);
    solidColorPolygon* ref=drawer->getByName<solidColorPolygon>("polygons");
    ref->staticBufferSetup(ref->meshIDs);
    ref->oneDynamicBufferSetup("modelMats", 1000*16, 4, 4, 2, 1);
    ploygonUniformIDS=ref->addNEntity(makevec3(data, 10), 2.0f);
    
    ref->updateDynamicBuffer("modelMats", 16, (float*)ref->u.model);
    gridDrawer* g=drawer->getByName<gridDrawer>("grids");
    g->staticBufferSetup(g->meshIDs);
    g->oneDynamicBufferSetup("translations", 10000*2, 2, 1, 1, 1);
    g->oneDynamicBufferSetup("scales", 10000*1, 1, 1, 2, 1);
    subGridIDs=g->makeMetaSquare(2.0f, 100);
    g->updateDynamicBuffer("translations", 2, (float*)g->u.trans);
    g->updateDynamicBuffer("scales", 1, (float*)g->u.scale);
    g->drawMode=GL_LINE_LOOP;
        

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
        g->setUniformFloat(getAlpha(dt, PI), "angle");
        g->setUniformFloat(observer::getInstance()->spike, "spike");
        g->setUniformFloat(observer::getInstance()->damp, "damp");
        drawer->draw();
        
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
            else if(e.key.keysym.sym==SDLK_UP)
            {
            }
            else if(e.key.keysym.sym==SDLK_DOWN)
            {
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