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

    void setStride(const int8_t s){this->stride=s;}
    void setPerVertexSize(const int8_t s){this->perVertexSize=s;}
    int8_t getPerVertexSize()const{return this->perVertexSize;}
    int8_t getStride()const{return this->stride;}
    size_t getSize()const{return this->size;}
    size_t getOffset()const{return this->offset;}
private:
    subBuffer()=default;
    subBuffer(const subBuffer&)=default;
    subBuffer(subBuffer&&)=default;
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
    :size(isize), datatype(dataType)
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
    void writeToSubBuffer(const int ID, const int w_size, const float* data, const int offset=0);

    //init a subbuffer with vertex specification, if data is provided a write is called aswell
    void init_subBuffer(const int ID, const int8_t iperVertexSize, const int8_t istride, float* data=nullptr, const int data_size=0);

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

struct subBufferHandle{

    subBufferHandle(const int iID, GPUbuffer* ptr)
    :subBufferID(iID), buffer(ptr){}
    subBufferHandle()
    :subBufferID(-1), buffer(nullptr){}

    int subBufferID;
    GPUbuffer* buffer;
};

struct gpuBufferList;
struct gpuBufferTree_head
{
    gpuBufferTree_head()
    :static_draw(GL_STATIC_DRAW, GL_FLOAT, 1000),
    dynamic_draw(GL_DYNAMIC_DRAW, GL_FLOAT, 1000),
    stream_draw(GL_STREAM_DRAW, GL_FLOAT, 1000)
    {}

    //floats
    gpuBufferList static_draw;
    gpuBufferList dynamic_draw;
    gpuBufferList stream_draw;
    //indices?
    //gpuBufferList* mesh_inidices;
};

struct gpuBufferList_node;

constexpr size_t MAX_BUFFERSIZE_SUM=1000000000/sizeof(float);
//idea: make a list of lists, where each list has an associated size
//opt: sort the list so we know where the full buffers are and dont check them
struct gpuBufferList
{

    gpuBufferList(GLenum idrawtype, GLenum itype, const size_t initial_size)
    :drawType(idrawtype), type(itype), next_bufferSize(initial_size)
    {}
    subBufferHandle newSubBuffer(const size_t n_size);

    subBufferHandle allocateNodeAndAddSubBuffer(gpuBufferList_node* ptr, const size_t size);
    gpuBufferList_node* allocateNewNode(const size_t size, GLenum type, GLenum drawType);

    //TODO() write this
    void deleteNodes();

    GLenum drawType;
    GLenum type;
    size_t next_bufferSize=1000;
    size_t combined_size=0;
    gpuBufferList_node* head=nullptr;
};

struct gpuBufferList_node
{
    gpuBufferList_node(const GPUbuffer ibuf)
    :buffer(ibuf){}

    GPUbuffer buffer;
    gpuBufferList_node* next=nullptr;
};


#define HASBIT64(nr, bit) (nr>>bit)&1
#define SETBIT64(nr, bit) nr&=(1<<bit)
#define NOTBIT64(nr, bit) nr&=!(1<<bit)
//#include <immintrin.h>
//#include <avx2intrin.h>


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

//IDEAS():
//  *un-link the construction and uploading of the data
//      this way you can init the data and upload it to gpu when it is needed
//  *make a function that produces a drawable, but hides everything that would be 
//      needed for that(like the unifrom container creation  etc.)
struct drawable{

protected:
    drawable(GLuint ishader, const std::vector<std::string> imeshIDs)
    :shader(ishader), meshIDs(imeshIDs) 
    {
        vertexCnt=meshContainer::getInstance()->getMesh(meshIDs[0]).getVertexCount();
    }
public:

    
    //int addEntity();
    void init(const size_t vertexCount, std::vector<oneVertexArraySetup> vertArrays);

    /*links a subBuffer into this->vao, attrib divisor is for instncaing, location is the location in this->shader, for the vertexAttrib in tha array*/
    void vertexArraySetup(const subBufferHandle buf, const int attribDivisor, const int location);

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



    //TODO grab the instance count from the uniform containers
    //this is not easyly done tho as it would have to happen every frame
    int instanceCount=0;
    size_t vertexCnt;//for drawing
    GLuint VAO=0;
    GLenum drawMode=GL_TRIANGLES;
    GLuint shader=0;

    std::vector<std::string> meshIDs;
};







#endif //GPUBUFFER_H

