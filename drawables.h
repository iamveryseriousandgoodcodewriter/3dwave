#ifndef DRAWABLES_H
#define DRAWABLES_H
#include "GPUbuffer.h"
#include "math/glm/vec3.hpp"
#include "math/glm/mat4x4.hpp"
#include "math/glm/ext/matrix_transform.hpp"
#include "math/glm/gtx/euler_angles.hpp"

void printmat4(glm::mat4 mat);

struct gridUniforms : public uniformContainer
{
    gridUniforms(const size_t count)
    :uniformContainer(count), trans(new glm::vec2[count+count%8]), scale(new float[count+count%8])
    {}
    int add(const glm::vec2 pos, const float iscale)
    {
        int id=uniformContainer::add();
        if(id==-1)
        {
            return -1;
        }
        trans[writeIndex-1]=pos;
        scale[writeIndex-1]=iscale;
        return id;
    }
    void sort()
    {
        auto vec2Sort=[](int i, int* gather, glm::vec2* data)
        {   
            glm::vec2 temp[8];
            for(char k=0;k<8;k++)
            {
                temp[k]=data[gather[k]];
            }
            memcpy(data+i, temp, 8*sizeof(glm::vec2));
        };
        auto floatSort=[](int i, int* gather, float* data)
        {
            float temp[8];
            for(char k=0;k<8;k++)
            {
                temp[k]=data[gather[k]];
            }
            memcpy(data+i, temp, 8*sizeof(float));
        };
        auto sortLmbd=[vec2Sort, floatSort, this](int i, int* gather){
            vec2Sort(i, gather, trans);
            floatSort(i, gather, scale);
        };

        sortWithData(sortLmbd);
    }

    glm::vec2* trans;
    float* scale;
};


struct gridDrawer;
void gridDrawerDrawCall(gridDrawer* t);
//template this with a uniform container?
struct gridDrawer:public drawable
{
    gridDrawer(const size_t imaxEntities, GPUbuffer* staticBuf, GPUbuffer* dynamicBuf, GLuint ishader, const std::vector<std::string> imeshIDs)
    :drawable(imaxEntities, staticBuf, dynamicBuf, ishader, imeshIDs), u(imaxEntities)
    {}

    //this stuff should be abstarcetd in the abse class
    void updateDynamicBuffer(const std::string buffer, int insFloatCnt, float* data)
    {
        this->u.sort();

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
        return this->u.add(itrans, iscale);
    }

    void removeEntity(const int ID)
    {
        this->u.remove(ID);
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

   

    gridUniforms u;
};
void gridDrawerDrawCall(gridDrawer* t)
{
    t->draw();
}

struct uniform3d : public uniformContainer
{
    uniform3d(const size_t isize)
    :uniformContainer(isize), trans(new glm::vec3[isize+isize%8]),rot(new glm::vec3[isize+isize%8]()), scale(new glm::vec3[isize+isize%8]),
    model(new glm::mat4[isize+isize%8])
    {}

    //TODO() realloc function if we get too big
    //TODO() update method

    int add()
    {
        int id=uniformContainer::add();
        if(id==-1)
        {
            return -1;
        }
        trans[writeIndex-1]=glm::vec3(0.0f,0.0f,0.0f);
        rot[writeIndex-1]=glm::vec3(0.0f,0.0f,0.0f);
        scale[writeIndex-1]=glm::vec3(1.0f,1.0f,1.0f);
        return id;
    }
    int add(const glm::vec3 itrans, const glm::vec3 irot, const glm::vec3 iscale)
    {
        int id=uniformContainer::add();
        if(id==-1)
        {
            return -1;
        }
        trans[writeIndex-1]=itrans;
        rot[writeIndex-1]=irot;
        scale[writeIndex-1]=iscale;
        return id;
    }

    void sort()
    {
        auto nestedSort=[](int i, int* gather, glm::vec3* data)
        {   
            glm::vec3 temp[8];
            for(char k=0;k<8;k++)
            {
                temp[k]=data[gather[k]];
            }
            memcpy(data+i, temp, 8*sizeof(glm::vec3));
        };
        auto sortLmbd=[nestedSort, this](int i, int* gather){
            nestedSort(i, gather, trans);
            nestedSort(i, gather, rot);
            nestedSort(i, gather, scale);
        };

        sortWithData(sortLmbd);
    }

    void buildModelMats()
    {
        int i;
        for(i=0;i<writeIndex;++i)
        {
            model[i]=glm::scale(glm::mat4(1.0f), scale[i]);
            
        }
        for(i=0;i<writeIndex;++i)
        {
            model[i]=glm::eulerAngleXYZ(rot[i].x,rot[i].y,rot[i].z)*model[i];
            
        }
        for(i=0;i<writeIndex;++i)
        {
            model[i]=glm::translate(model[i], trans[i]);
            //printf("\n printing mat %i", i);
            //printmat4(model[i]);
        }
    }
   

    //put the responsibility of gpu buffers on this?

    //build matrix or leave raw?
    glm::vec3* trans;
    glm::vec3* rot;
    glm::vec3* scale;

    glm::mat4* model;
};


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

    solidColorPolygon(const size_t imaxEntities, GPUbuffer* staticBuf, GPUbuffer* dynamicBuf, GLuint ishader, const std::vector<std::string> imeshIDs)
    :drawable(imaxEntities, staticBuf, dynamicBuf, ishader, imeshIDs), u(imaxEntities)
    {}

    void updateDynamicBuffer(const std::string buffer, int insFloatCnt, float* data)
    {
        this->u.sort();
        this->u.buildModelMats();

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
        return this->u.add(itrans, irot, iscale);
    }

    void removeEntity(const int ID)
    {
        this->u.remove(ID);
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

    uniform3d u;
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