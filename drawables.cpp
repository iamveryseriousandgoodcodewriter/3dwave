#include "drawables.h"


/*
================================================================================
================================================================================
================================================================================
==================================DRAWABLE======================================
================================================================================
================================================================================
================================================================================
================================================================================
*/


void drawable::init(const size_t vertexCount, uniformContainer* entityArray, std::vector<oneVertexArraySetup> vertArrays)
{
    myEntities=entityArray;
    this->vertexCnt=vertexCount;
    for(auto& it:vertArrays)
    {
        this->vertexArraySetup(it.han, it.attribDivisor, it.location);
    }
}

int attribPtrCntHelper(const int perVertexSize);
int vertexVectorSizeHelper(const int size);
void drawable::vertexArraySetup(const subBufferHandle buf, const int attribDivisor, const int location)
{
    if(!buf.buffer)
    {DEBUGMSG("\n vertexArraySetup failed: buffer is NULL");return;}

    subBuffer& sb=buf.buffer->getSubBuffer(buf.subBufferID);

    if(sb.getPerVertexSize()==-1)
    {DEBUGMSG("\n vertexArray setup failed: subBuffer uninitialized");return;}

    const int attribPtrCnt=attribPtrCntHelper(sb.getPerVertexSize());
    const int perVertexSize=sb.getPerVertexSize();
    const size_t stride=sb.getPerVertexSize()*sizeof(float);
    const size_t globalOffset=sb.getOffset()*sizeof(float);

    if(!glIsVertexArray(VAO))
    {
        glGenVertexArrays(1, &this->VAO);
    }
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, buf.buffer->bufferID);

    for(int i=0; i<attribPtrCnt;++i)
    {
        glEnableVertexAttribArray(location+i);
        glVertexAttribPointer(location+i, 
            vertexVectorSizeHelper(perVertexSize-(4*(i+1))), 
            buf.buffer->datatype, GL_FALSE, 
            stride, 
            (void*)(4*i*sizeof(float)+globalOffset)
        );
        glVertexAttribDivisor(location+i, attribDivisor);
    }

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
int attribPtrCntHelper(const int perVertexSize)
{
    if(!perVertexSize%4)
    {
        return perVertexSize/4;
    }
    else
    {
        return (perVertexSize/4)+1;
    }
}
int vertexVectorSizeHelper(const int size)
{
    if(size>=0)return 4;
    else return 4+size;
    
}


void drawable::setUniformFloat(const float f, const char* name)const
{
    glUseProgram(this->shader);
    glUniform1f(glGetUniformLocation(this->shader, name), f);
    glUseProgram(0);
}
void drawable::draw()const
{
    glUseProgram(this->shader);
    glBindVertexArray(this->VAO);
    glDrawArraysInstanced(this->drawMode, 0, vertexCnt, this->instanceCount);
    //glDrawArrays(this->drawMode, 0, vertexCnt);
    glBindVertexArray(0);
}




