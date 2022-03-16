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


// TODO() make everything nice and shiny(with loading functions and stuff like that)


struct drawable{

    drawable(GLuint ishader)
    :shader(ishader)
    {}
public:

    //TODO():rework init and vertexArraySetup to match their use
    //  both function below dont really get used in their intended way
    //  by the construct drawable method

    /*
    init the drawable, so it can be used
    vertexCount: specifies the nr of vertexes to be drawn per instance
    entityArray: unifromContainer associated with this drawable
                 the add functions work on this one
    vertArrays: calls vertexArraySetup on these, in the order
                they are given
                Locations are linked to the Shader!
    */   
    void init(const size_t vertexCount, uniformContainer* entityArray, std::vector<oneVertexArraySetup> vertArrays);

    /*
    links a subBuffer into this->vao, 
    attrib divisor: is for instncaing, 
    location: location for this attrib in this->shader
                if your vertexArray is >4 float location is the first one
                (same es in the shader)
    */
    void vertexArraySetup(const subBufferHandle buf, const int attribDivisor, const int location);



    /*
        set the uniform in this->shader
        TODO(): make this more useable, linked to reowrk of master
                container, look in ideas file
    */
    void setUniformFloat(const float f, const char* name)const;

    //.........
    void draw()const;
    

    /*
    add one Entity to this Drawable, returns its ID
    Entity: the singleEn. struct for the container this drawable
            uses. make sure they are the same(uniformCont::typeID)
    */
    template<typename cont, typename single>
    int addEntity(const single Entity);

    /*
        same as addEntity but in bulk
    */
    template<typename cont, typename single>
    std::vector<int> addNEntity(const std::vector<single> NEntity);

    //grabs the current instance Count from the associated uniformContainer
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




extern const GLchar* solidColorCubeVertexShader;
extern const GLchar* solidColorCubeFragmentShader;
extern const GLchar* gridShaderWithWave_vertex;


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