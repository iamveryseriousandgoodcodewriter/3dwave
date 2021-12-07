#ifndef DRAWABLES_H
#define DRAWABLES_H
#include "GPUbuffer.h"
#include "math/glm/vec3.hpp"
#include "math/glm/mat4x4.hpp"
#include "math/glm/ext/matrix_transform.hpp"
#include "math/glm/gtx/euler_angles.hpp"
#include "dynamicRenderData.h"


inline
void printmat4(glm::mat4 mat)
{
    std::cout<<'\n'<<mat[0][0]<<mat[1][0]<<mat[2][0]<<mat[3][0];
    std::cout<<'\n'<<mat[0][1]<<mat[1][1]<<mat[2][1]<<mat[3][1];
    std::cout<<'\n'<<mat[0][2]<<mat[1][2]<<mat[2][2]<<mat[3][2];
    std::cout<<'\n'<<mat[0][3]<<mat[1][3]<<mat[2][3]<<mat[3][3];
}

struct oneVertexArraySetup
{
    oneVertexArraySetup(const subBufferHandle sbh, int idiv, int iloc)
    :han(sbh), attribDivisor(idiv), location(iloc){}
    subBufferHandle han;
    int attribDivisor;
    int location;
};



//this is works like this currently:
//you make meshes, you put the meshes in the gpuBuffer
//you make instances(uniforms), and put them in the GPUBuffer
//    *this is by inheritance in the uniform_impl.h
//you grab the subBufferhandles and call this->init with them
//      *you have to know the attribDiv(obv)
//      *the location is based on the shader you are using
//you draw
//:))

//TODO():
//  rework the drawable container
//  make the pipeline concept from the uniform coantainer more generic?
//  make an example and run it
//  build a better abstraction for the uniformBufferObjects
//  make everything nice and shiny(with loading functions and stuff like that)

//hold all obj_containers in some meta way
//add "relation-vector" to it, where you can specify 2 of these
//containers and a way they want to interact(like compute collisions)


//IDEAS():
//  *un-link the construction and uploading of the data
//      this way you can init the data and upload it to gpu when it is needed
//  *make a function that produces a drawable, but hides everything that would be 
//      needed for that(like the unifrom container creation  etc.)
// maybe the drawable wants to hold all of the construction nessecary for it?
//maybe not tho lol
struct drawable{

    drawable(GLuint ishader)
    :shader(ishader)
    {}
public:

    //both function below dont really get used in their intended way
    //   by the construct drawable method
    //  so rework those i guess
    
    //int addEntity();
    void init(const size_t vertexCount, uniformContainer* entityArray, std::vector<oneVertexArraySetup> vertArrays);

    /*links a subBuffer into this->vao, attrib divisor is for instncaing, location is the location in this->shader, for the vertexAttrib in tha array*/
    void vertexArraySetup(const subBufferHandle buf, const int attribDivisor, const int location);

    void setUniformFloat(const float f, const char* name)const;


    void draw()const;
    

    template<typename cont, typename single>
    int addEntity(const single Entity);
    template<typename cont, typename single>
    std::vector<int> addNEntity(const std::vector<single> NEntity);

    void getInstanceCount(){this->instanceCount=myEntities->end();}
    
    //TODO grab the instance count from the uniform containers
    //this is not easyly done tho as it would have to happen every frame
    uniformContainer* myEntities=nullptr;
    int instanceCount=0;
    size_t vertexCnt;//for drawing
    GLuint VAO=0;
    GLenum drawMode=GL_TRIANGLES;
    GLuint shader=0;
};


struct drawables_list : public pipeline{

    drawable& add(std::string name, drawable d)
    {
        drawables.insert({name, d});
        return drawables.at(name);
    }

    std::function<void(const float)> getPipe()
    {

        return [this](const float){this->getInstanceCountAndDraw();};
    }

    void getInstanceCountAndDraw()
    {
        for(auto& it: drawables)
        {
            it.second.getInstanceCount();
            it.second.draw();
        }
    }

    std::unordered_map<std::string, drawable> drawables;
};


template<typename cont, typename single>
int drawable::addEntity(const single Entity)
{
    if(cont::myID!=myEntities->getTypeID())
        return -1;
    return ((cont*)myEntities)->add(Entity);
}
template<typename cont, typename single>
std::vector<int> drawable::addNEntity(const std::vector<single> NEntity)
{
    if(cont::myID!=myEntities->getTypeID())
        return std::vector<int>(-1);
    return ((cont*)myEntities)->addN(NEntity);
}




const GLchar* solidColorCubeVertexShader=R"glsl(
    #version 420 core
    layout (location=0) in vec3 pos;
    layout (location=1) in vec4 color;
    layout (location=2) in mat4 model;

    //global camera mat
    layout (std140, binding = 0) uniform cameraMat
    {
        mat4 projection;
        mat4 view;
    }camera;

    out vec4 v_out_color;

    void main()
    {
        
        gl_Position=camera.projection*camera.view*model*vec4(pos, 1.0f);
        v_out_color=color;
    }
)glsl";


const GLchar* solidColorCubeFragmentShader=R"glsl(
    #version 420 core

    in vec4 v_out_color;

    out vec4 final_color;
    
    void main()
    {
        final_color=v_out_color;
    }

)glsl";

const GLchar* gridShaderWithWave_vertex=R"glsl(
    #version 420 core

    layout (location=0) in vec2 pos;
    layout (location=1) in vec2 trans;
    layout (location=2) in float scale;

    uniform float angle;
    uniform float damp;//the smaller this is the more dampend
    uniform float spike;//the smaller this is the higher they spike

    layout (std140, binding = 0) uniform cameraMat
    {
        mat4 projection;
        mat4 view;
    }camera;

    layout(std140, binding = 10) uniform waveOrigin
    {
        vec3 posPhase[10];
    }origin;


    out vec4 v_out_color;

    float getAccZ(in float posx, in float posy, in float alpha, in vec3 org)
    {

       vec2 originDis=vec2(posx,posy)-vec2(org.x, org.y);

       float r=sqrt(originDis.x*originDis.x+originDis.y*originDis.y);
        
       return -sin(r-alpha+org.z)*(damp/(r+spike));
    }

    void main()
    {


        vec4 final_pos=vec4(1.0f,1.0f,0.0f,1.0f);
        final_pos.x=(pos.x*scale)+trans.x;
        final_pos.y=(pos.y*scale)+trans.y;
        for(int i=0;i<10;++i)
        {
            final_pos.z+=getAccZ(final_pos.x, final_pos.y, angle, origin.posPhase[i]);
        }
        gl_Position=camera.projection*camera.view*final_pos;

        v_out_color=vec4(0.75f, 0.75f, 0.75f, 0.75f);
    }


)glsl";


//shared ubo
//this is for like the wave origin
struct UBOwrapper
{
    //binding is hardcoded in shader, buffersize is in floats
    UBOwrapper(const std::string iname, const GLuint binding, const size_t buffersize)
    :size(buffersize), name(iname)
    {
        for(auto it: bindingPoints)
        {
            if(binding==it)
            {
                //TODO()throw exception or whatever?
                DEBUGMSG("\n UBOwrapper constructor: binding point already taken");
            }
        }
        bindingPoint=binding;
    }

    void init(GLenum drawType)
    {
        glGenBuffers(1, &glID);
        glBindBuffer(GL_UNIFORM_BUFFER, glID);
        glBufferData(GL_UNIFORM_BUFFER, size*sizeof(float), NULL, drawType);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, glID);
    }

    void updateUBO(const void* data)
    {
        glBindBuffer(GL_UNIFORM_BUFFER, glID);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, size*sizeof(float), data);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    std::string name;
    GLuint glID=0;
    GLuint bindingPoint;
    size_t size=0;//size of the buffer in floats, RESPECT PADDING

    static std::vector<GLuint> bindingPoints;
};


#endif //DRAWABLES_H