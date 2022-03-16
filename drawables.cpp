#include "drawables.h"





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
    glBindBuffer(GL_ARRAY_BUFFER, buf.buffer->getBufferID());

    for(int i=0; i<attribPtrCnt;++i)
    {
        glEnableVertexAttribArray(location+i);
        glVertexAttribPointer(location+i, 
            vertexVectorSizeHelper(perVertexSize-(4*(i+1))), 
            buf.buffer->getDataType(), GL_FALSE, 
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
    if(!(perVertexSize%4))
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



