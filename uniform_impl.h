#ifndef UNIFORM_IMPL_H
#define UNIFORM_IMPL_H
#include "masterContainer.h"




struct uniform3d : public uniformContainer
{
    uniform3d(const size_t isize, std::string name)
    :uniformContainer(isize),trans(new glm::vec3[isize+isize%8]),rot(new glm::vec3[isize+isize%8]()), scale(new glm::vec3[isize+isize%8]),
    model(new glm::mat4[isize+isize%8])
    {

        if(!trans||!scale||!rot||!model)
        {
            throw bad_construction_exe(bad_construction_exe::bad_alloc,
            "uniform3d allocation failed with size: "+std::to_string(isize));
        }
        subBufferHandle sbh=masterContainer::_GPUBuffers.dynamic_draw.newSubBuffer(isize*16);
        if(!sbh.buffer)
        {
            throw bad_construction_exe(
                bad_construction_exe::bad_gpu_subBufferHandle,
                "newSubBUffer on dynamic_draw returned null, on uniform3d construction with size:"+std::to_string(isize));
        }
        this->myGPUBuffer.push_back(sbh);
        this->myGPUBuffer[0].buffer->init_subBuffer(this->myGPUBuffer[0].subBufferID,
        16, 0);
        masterContainer::subBuffers.insert({name, myGPUBuffer[0]});
    }

    void reAllocate(const size_t newSize);
    void remove(const int ID)
    {
        uniformContainer::remove(ID);
    }
    
    //TODO() realloc function if we get too big
    //TODO() update method

    int add();
    int add(const glm::vec3 itrans, const glm::vec3 irot, const glm::vec3 iscale);

    void internalUpdate()
    {
        this->buildModelMats();
    }

    void onUpload()
    {
        this->myGPUBuffer[0].buffer->writeToSubBuffer(
            this->myGPUBuffer[0].subBufferID,
             this->writeIndex*16,
              (float*)model);
    }
    
    uniformContainer_list::pipelineFuncPtrs makeEntryTicket()
    {
        uniformContainer_list::pipelineFuncPtrs p;
        p.sort=std::bind(&uniform3d::sort, std::ref(*this));
        p.dt=nullptr;
        p.internal=std::bind(&uniform3d::buildModelMats, std::ref(*this));
        p.GPU_upload=std::bind(&uniform3d::onUpload, std::ref(*this));
        return p;
    }

    void sort();

    void buildModelMats();


    //build matrix or leave raw?
    glm::vec3* trans;
    glm::vec3* rot;
    glm::vec3* scale;

    glm::mat4* model;
};



struct gridUniforms : public uniformContainer
{
    gridUniforms(const size_t count, std::vector<std::string> names)
    :uniformContainer(count), trans(new glm::vec2[count+count%8]), scale(new float[count+count%8])
    {
        //trans
        if(!trans||!scale)
        {
            throw bad_construction_exe(bad_construction_exe::bad_alloc,
            "gridUniforms allocation failed with size: "+std::to_string(count));
        }
        subBufferHandle sbh=masterContainer::_GPUBuffers.static_draw.newSubBuffer(count*2);
        if(!sbh.buffer)
        {
            throw bad_construction_exe(
                bad_construction_exe::bad_gpu_subBufferHandle,
                "newSubBUffer on static_draw returned null, on gridUniforms::trans construction with size:"+std::to_string(count));
        }
        this->myGPUBuffer.push_back(sbh);
        this->myGPUBuffer[0].buffer->init_subBuffer(this->myGPUBuffer[0].subBufferID, 2, 0);
        masterContainer::subBuffers.insert({names[0], myGPUBuffer[0]});

        //scale
        sbh=masterContainer::_GPUBuffers.static_draw.newSubBuffer(count);
        if(!sbh.buffer)
        {
            throw bad_construction_exe(
                bad_construction_exe::bad_gpu_subBufferHandle,
                "newSubBUffer on static_draw returned null, on gridUniforms::scale construction with size:"+std::to_string(count));
        }
        this->myGPUBuffer.push_back(sbh);
        this->myGPUBuffer[1].buffer->init_subBuffer(this->myGPUBuffer[1].subBufferID, 1, 0);
        masterContainer::subBuffers.insert({names[1], myGPUBuffer[1]});

    }
    int add(const glm::vec2 pos, const float iscale);
    void sort();
    void remove(const int ID)
    {
        uniformContainer::remove(ID);
    }

    void onGPUUpload()
    {
        //IDEA: "buffer" these write calls, and perform them on the list in bulk
        this->myGPUBuffer[0].buffer->writeToSubBuffer(
            this->myGPUBuffer[0].subBufferID,
             this->writeIndex*2,
              (float*)trans);

        this->myGPUBuffer[1].buffer->writeToSubBuffer(
            this->myGPUBuffer[1].subBufferID,
             this->writeIndex,
              (float*)scale);
    }
    
    uniformContainer_list::pipelineFuncPtrs makeEntryTicket(){
        uniformContainer_list::pipelineFuncPtrs p;
        p.sort=std::bind(&gridUniforms::sort, std::ref(*this));
        p.dt=nullptr;
        p.internal=nullptr;
        p.GPU_upload=std::bind(&gridUniforms::onGPUUpload, std::ref(*this));
        return p;
    };
    
    glm::vec2* trans;
    float* scale;
};

#endif //UNIFORM_IMPL_H