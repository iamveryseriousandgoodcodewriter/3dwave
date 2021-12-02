#ifndef DRAWABLES_H
#define DRAWABLES_H
#include "GPUbuffer.h"
#include "math/glm/vec3.hpp"
#include "math/glm/mat4x4.hpp"
#include "math/glm/ext/matrix_transform.hpp"
#include "math/glm/gtx/euler_angles.hpp"
#include "dynamicRenderData.h"

void printmat4(glm::mat4 mat);




struct gridDrawer;
void gridDrawerDrawCall(gridDrawer* t);
//template this with a uniform container?
struct gridDrawer:public drawable
{
    gridDrawer(const size_t imaxEntities, GPUbuffer* staticBuf, GPUbuffer* dynamicBuf, GLuint ishader, const std::vector<std::string> imeshIDs, gridUniforms* uni_ptr)
    :drawable(imaxEntities, staticBuf, dynamicBuf, ishader, imeshIDs), u(uni_ptr)
    {}

    //this stuff should be abstarcetd in the abse class
    void updateDynamicBuffer(const std::string buffer, int insFloatCnt, float* data)
    {
        this->u->sort();

        int bufferID=dynamicSubBuffers.at(buffer);
        size_t w_size=instanceCount*insFloatCnt;
        this->dynamicBuffer->writeToSubBuffer(bufferID, 0, w_size, data);               
    }

    int addEntity(const glm::vec2 itrans, const float iscale)
    {
        if(instanceCount==maxEntities)
        {
            DEBUGMSG("\ndrawables container full, cant add");
            return -1;
        }
        this->instanceCount++;
        return this->u->add(itrans, iscale);
    }

    void removeEntity(const int ID)
    {
        this->u->remove(ID);
        instanceCount--;
    }

    std::vector<int> makeMetaSquare(const float meshScale, const int a)
    {
        glm::vec2 currPos(0.0f,0.0f);
        std::vector<int> ret;
        float min=-meshScale*(float)a/2;
        for(int i=0;i<a;++i)
        {
            for(int k=0; k<a;++k)
            {
                currPos=glm::vec2(min+i*meshScale, min+k*meshScale);
                ret.push_back(addEntity(currPos, meshScale));
            }
        }
        
        return ret;
    }

    std::function<void()> getDrawFunc()
    {
        return std::bind(&gridDrawerDrawCall, this);
    }

   

    gridUniforms* u;
};
void gridDrawerDrawCall(gridDrawer* t)
{
    t->draw();
}




void printmat4(glm::mat4 mat)
{
    std::cout<<'\n'<<mat[0][0]<<mat[1][0]<<mat[2][0]<<mat[3][0];
    std::cout<<'\n'<<mat[0][1]<<mat[1][1]<<mat[2][1]<<mat[3][1];
    std::cout<<'\n'<<mat[0][2]<<mat[1][2]<<mat[2][2]<<mat[3][2];
    std::cout<<'\n'<<mat[0][3]<<mat[1][3]<<mat[2][3]<<mat[3][3];
}


struct solidColorPolygon;
void solidColorPolygonDrawCall(solidColorPolygon* t);
struct solidColorPolygon : public drawable{

    solidColorPolygon(const size_t imaxEntities, GPUbuffer* staticBuf, GPUbuffer* dynamicBuf, GLuint ishader, const std::vector<std::string> imeshIDs, uniform3d* uni_ptr)
    :drawable(imaxEntities, staticBuf, dynamicBuf, ishader, imeshIDs), u(uni_ptr)
    {}

    void updateDynamicBuffer(const std::string buffer, int insFloatCnt, float* data)
    {
        //this should be done in the pipeline?
        //this->u.sort();
        //this->u.buildModelMats();

        int bufferID=dynamicSubBuffers.at(buffer);
        size_t w_size=instanceCount*insFloatCnt;
        this->dynamicBuffer->writeToSubBuffer(bufferID, 0, w_size, data);               
    }

    int addEntity(const glm::vec3 itrans, const glm::vec3 irot, const glm::vec3 iscale)
    {
        if(instanceCount==maxEntities)
        {
            DEBUGMSG("\ndrawables container full, cant add");
            return -1;
        }
        this->instanceCount++;
        return this->u->add(itrans, irot, iscale);
    }

    void removeEntity(const int ID)
    {
        this->u->remove(ID);
        instanceCount--;
    }


    std::vector<int> addNEntity(std::vector<glm::vec3> pos, float scale)
    {
        std::vector<int> ret;
        for(int i=0;i<pos.size();++i)
        {
            ret.push_back(
                this->addEntity(
                    pos[i],
                    glm::vec3(0,0,0),
                    glm::vec3(scale, scale, scale)
                )
            );
        }
        return ret;
    }

    std::function<void()> getDrawFunc()
    {
        return std::bind(&solidColorPolygonDrawCall, this);
    }

    uniform3d* u;
};
void solidColorPolygonDrawCall(solidColorPolygon* t)
{
    t->draw();
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