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