#ifndef GPUBUFFER_H
#define GPUBUFFER_H
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <vector>
#include <stdio.h>
#include <string.h>
#include <numeric>
#include <algorithm>
#include <functional>
#include <utility>
#include <tuple>
#include "utils.h"
#include "mesh.h"




//1. request buffer from GPU-buffer store
//2. make subbuffer in it, with a certain mesh
//3. build vaos from subbuffers i just made




struct subBuffer{
    friend struct GPUbuffer;
    
    void print(const char* msg="")const
    {
        printf("\n%s  ", msg);
        printf("subBuffer--->ID:%i, size: %ld, offset: %ld", ID, size, offset);
    }

    void setStride(int8_t s){this->stride=s;}
    void setPerVertexSize(int8_t s){this->perVertexSize=s;}
    int8_t getPerVertexSize(){return this->perVertexSize;}
    int8_t getStride(){return this->stride;}
    size_t getSize(){return this->size;}
    size_t getOffset(){return this->offset;}
private:
    int8_t perVertexSize=-1;
    int8_t stride=-1;
    int ID=-1;
    int refCount=0;
    size_t size=0;//size, alos in floats
    size_t offset=0;//offset into parentbuffer, infloats
};



//this is float only. fuck off with them ints. also this is a hot buffer
//meant to be middle sized with the ability to sccess sub ranges
//for big boy stuff maybe to a AoS approach?
struct GPUbuffer{

    GPUbuffer(const size_t isize, const GLenum dataType)
    :size(isize), datatype(dataType), subBuffers(std::vector<subBuffer>())
    {}

    bool init(const GLenum drawType=GL_STATIC_DRAW);


    /*
    warning: this is expensive
    finds an offset within the buffer, where the new requested buffer of size nsize would fit in, and returns the offset.
    !!!!n_size and return value are in floats/datatype!!!!
    returns error codes:
    -1: n_size > this->size
    -2: not enough space left
    -3: cant find a small enough space to fit in n_size, either sort or find     other buffer
    */
    int getOffsetForSize(const size_t n_size);

    int newSubBuffer(const size_t n_size);
    subBuffer& getSubBuffer(const int ID);

    //write to buffer ID, offset into the subbuffer, size in floats
    void writeToSubBuffer(const int ID, int offset, int w_size, float* data);

    void sortSubBuffers();
    //TODO()
    //write a function to repack the buffer in the GPU
    void repackSubBuffers();

    int getSameSubBuffer(const int ID);
    void freeSubBuffer(const int ID);

    GLenum drawtype=GL_FALSE;
    GLenum datatype=GL_FLOAT;
    int IDcounter=0;
    size_t size=0;//in floats, i guess
    size_t freeSize;
    GLuint bufferID=0;
    std::vector<subBuffer> subBuffers;
};

#define HASBIT64(nr, bit) (nr>>bit)&1
#define SETBIT64(nr, bit) nr&=(1<<bit)
#define NOTBIT64(nr, bit) nr&=!(1<<bit)
//for each subbuffer id: query a draw call with a uniform, upload the uniforms into a buffer
//and then call an instanced render method on that subrange
//#include <immintrin.h>
//#include <avx2intrin.h>







struct drawable{

protected:
    drawable(const size_t imaxEntities, GPUbuffer* staticBuf, GPUbuffer* dynamicBuf, GLuint ishader, const std::vector<std::string> imeshIDs)
    :maxEntities(imaxEntities), staticBuffer(staticBuf), dynamicBuffer(dynamicBuf),shader(ishader), meshIDs(imeshIDs) 
    {
        vertexCnt=meshContainer::getInstance()->getMesh(meshIDs[0]).getVertexCount();
    }
public:

    
    int addEntity();
    void init();

    //this uses the msehcontainer
    void staticBufferSetup(std::vector<std::string> meshes);

    void oneStaticBufferSetup();

    /*  
        make a subbuffer in dynamic buffer, save in map with name
        it will hold the floats specified
        perVertSize is the number of elements per vertex
        attribPtrCnt is the number of attribptrs for this buffer
                    (if you put mat4 you need 4ptrs*4vertsize)
        attribDivisor is for instancing (1 means advance once per instance)
        location begin is the location in the shader
    */
    void oneDynamicBufferSetup(const std::string name,const size_t sizeInFloats, const int perVertSize, const int attribPtrCnt, const int locationBegin, const int attribDivisor);


    void setUniformFloat(const float f, const char* name)
    {
        glUseProgram(this->shader);
        glUniform1f(glGetUniformLocation(this->shader, name), f);
        glUseProgram(0);
    }


    void draw()const
    {
        glUseProgram(this->shader);
        glBindVertexArray(this->VAO);
        glDrawArraysInstanced(this->drawMode, 0, vertexCnt, this->instanceCount);
        glBindVertexArray(0);
    }

    std::function<void()> getDrawFunc()const;
    //method to switch between static/dynamic buffers?
    //how to copy the data?
    //need to update VAO asweel

    size_t maxEntities=0;
    int instanceCount=0;
    //uniforms entities;

    GLenum drawMode=GL_TRIANGLES;
    //multiple vaos with a drawmode arg?
    GLuint VAO=0;
    size_t vertexCnt;//for drawing, also needs the offset tho
    GPUbuffer* staticBuffer;
    std::unordered_map<std::string, int> staticSubBuffers;//these could arguably hold the subbuffers themselfs
    GPUbuffer* dynamicBuffer;
    std::unordered_map<std::string, int> dynamicSubBuffers;

    std::vector<std::string> meshIDs;

    GLuint shader=0;
};







#endif //GPUBUFFER_H

