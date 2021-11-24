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



struct subBuffer{

    void print(const char* msg="")const
    {
        printf("\n%s  ", msg);
        printf("subBuffer--->ID:%i, size: %ld, offset: %ld", ID, size, offset);
    }

    int ID=-1;
    int refCount=0;
    size_t size=0;//size, alos in floats
    size_t offset=0;//offset into parentbuffer, infloats
};



//this is float only. fuck off with them ints. also this is a hot buffer
//meant to be middle sized with the ability to sccess sub ranges
//for big boy stuff maybe to a AoS approach?
struct GPUbuffer{

    //appears to be working on a cpu level
    //next steps:
    //make functions for actually writing in the buffer

    GPUbuffer(const size_t isize)
    :size(isize), subBuffers(std::vector<subBuffer>())
    {}

    bool init(const GLenum drawType=GL_STATIC_DRAW);

    int getSameSubBuffer(const int ID);
    //warning: this is expensive
    int newSubBuffer(const size_t n_size);//in floats

    //write to buffer ID, offset into the subbuffer, size in floats
    void writeToSubBuffer(const int ID, int offset, int w_size, float* data);

    void sortSubBuffers();
    //TODO()
    //write a function to repack the buffer in the GPU
    void repackSubBuffers();

    void freeSubBuffer(const int ID);

    GLenum drawtype=GL_FALSE;
    int IDcounter=0;
    size_t size=0;//in floats, i guess
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


struct uniformContainer{
    uniformContainer( const size_t isize)
    :entitySize(isize+isize%8),  entityIDs(new int[isize+isize%8]())
    {
        for(size_t i=0;i<entitySize;++i)
        {
            entityIDs[i]=-1;
        }
    }

    //TODO(): implement rule of 5, or use uniqueptr

    //now adds are fast and rmv is slow
    //if you guarantee a sorted list of ints
    //(th sort algo respects the order of ints)
    //you can binary search for ID to remove O(log n)
    int add();

    void remove(const int ID);


    void sortWithData(std::function<void(int i, int* gather)> subSorts); 


    ~uniformContainer()
    {
        delete[] entityIDs;
    }
    size_t size()const{return entitySize;}
    int end()const{return writeIndex;}
protected:
    int IDCounter=0;
    size_t entitySize;
    int* entityIDs;
    int writeIndex=0;
public:
    void printIDs(const char* msg="")const
    {
        printf("\n%s", msg);
        for(int i=0; i<writeIndex; ++i)
        {
            printf("\nId: %i, vIndex:%i", entityIDs[i], i);
        }
    }
};




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

/*
    void sort(const size_t uniformSize=0, void* uniforms=nullptr)
    {
        int i, k=1, end;
        //int64_t temp;
        for(i=0; i<keyCount;)
        {
            if(dataKey[i]==0)
            {
                dataKey[i]=dataKey[keyCount-k++];
                memcpy(entityIDs+i*64, entityIDs+(keyCount-k)*64, sizeof(unsigned int)*64);
                if(uniforms)//for le safety
                {
                    memcpy((char*)uniforms+i*uniformSize*64, (char*)uniforms+(keyCount-k)*64*uniformSize, uniformSize*64);
                }
                dataKey[keyCount-k++]=0;
            }
        }
        end=keyCount-k;
        //keycount-k=last key with some data in it

        auto swap=[&](int64_t to, int l, int64_t from, int j){
            
            if(uniforms)
            {
                memcpy((char*)uniforms+(to*64+l)*uniformSize, (char*)uniforms+((from*64)+j)*uniformSize, uniformSize);
            }
            entityIDs[to*64+l]=entityIDs[from*64+j];
            //return void;
        };

        int k_end=63;
        for(i=0;i<end;++i)
        {
            for(k=0;k<64&&i<end;k++)
            {   
                if( !(HASBIT64(dataKey[i], k)) )//find bit that is 0 in beginning
                {
                    while(i<=end&&k<k_end)
                    {
                        if(k_end<0)
                        {
                            k_end=63;
                            end--;
                        }
                        if(HASBIT64(dataKey[end], k_end--))//we found a bit in the end
                        {
                            //swap the bits
                            SETBIT64(dataKey[i], k);
                            NOTBIT64(dataKey[end], (k_end+1));
                            swap(i, k, end, k_end);
                            break;
                        }
                        
                    }
                }
            }
        }
        
    }
    */