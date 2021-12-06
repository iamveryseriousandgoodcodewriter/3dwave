#ifndef UNIFORM_IMPL_H
#define UNIFORM_IMPL_H
#include "masterContainer.h"


struct oneUniform3d{
    oneUniform3d(const glm::vec3 t, const glm::vec3 r, const glm::vec3 s)
    :trans(t), rot(r), scale(s)
    {}
    glm::vec3 trans;
    glm::vec3 rot;
    glm::vec3 scale;
};
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

    void remove(const int ID)
    {
        uniformContainer::remove(ID);
    }
    
    //TODO() realloc function if we get too big
    //TODO() update method

    void reAllocate(const size_t newSize)
    {
        if(newSize<writeIndex)
        {
            DEBUGMSG("\n unfiromContainer reAlloc failed: trying to shrink w    writeIndex");
            return;
        }
        trans=allocateAndCopy(newSize, trans, writeIndex);
        rot=allocateAndCopy(newSize, rot, writeIndex);
        scale=allocateAndCopy(newSize, scale, writeIndex);

    }

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
    int add(const glm::vec3 itrans, const glm::vec3 irot, const glm::vec3    iscale)
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
    std::vector<int> addN(
        const std::vector<glm::vec3> itrans, 
        const std::vector<glm::vec3> iscale, 
        const std::vector<glm::vec3> irot
    )
    {
        std::vector<int> ret;
        int curr, i;
        for(i=0;i<itrans.size(); ++i)
        {
            if((curr=uniformContainer::add())==-1)
            {
                DEBUGMSG("\n uniformcontainer full! addN aborted!");
                break;
            }
            ret.push_back(curr);
            trans[writeIndex-1]=itrans[i];
            scale[writeIndex-1]=iscale[i];
            rot[writeIndex-1]=irot[i];
        }
        return ret;
    }
    std::vector<int> addN(const std::vector<oneUniform3d> entity)
    {
        std::vector<int> ret;
        int curr, i;
        for(i=0;i<entity.size(); ++i)
        {
            if((curr=uniformContainer::add())==-1)
            {
                DEBUGMSG("\n uniformcontainer full! addN aborted!");
                break;
            }
            ret.push_back(curr);
            trans[writeIndex-1]=entity[i].trans;
            scale[writeIndex-1]=entity[i].scale;
            rot[writeIndex-1]=entity[i].rot;
        }
        return ret;
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
    //makes a big grid of little small grid, each of scale
    //side is how big one side should be
    //if left -1 the whole container will be used
    std::vector<int> makeDiagonalSquare(const float scale, int middle=-1)
    {
        std::vector<int> ret;
        //check the maximum middle size that fits
        int max=entitySize-writeIndex;
        int i=0, currMiddle=1;
        while(max-currMiddle>0)
        {
            currMiddle+=2;
            max-=(i+=2)*2;
        }

        //check if we exeed that
        int count;
        if(middle==-1||middle>currMiddle)
        {
            middle=currMiddle;
        }

        //makes a line of lineSize, offset into y-dir by scale*y
        auto lineBuilder=[scale, this, &ret](int lineSize, int y){
            ret.push_back(add({0.0f,scale*y}, scale));
            for(int i=1, k=1;i<lineSize;i+=2, k++)
            {
                ret.push_back(add({scale*k, scale*y}, scale));
                ret.push_back(add({-scale*k, scale*y}, scale));
            }
        };
        
        //builds the grid from lines recursivly
        std::function<int(int,int)> builder=
        [lineBuilder, &builder](int lineSize, int y)->int{
            if(lineSize<1)
            {
                return lineSize;
            }
            else
            {
                lineBuilder(lineSize, y);
                lineBuilder(lineSize, -y);
                return builder(lineSize-2, y+1);
            }
        };

        lineBuilder(middle, 0);
        builder(middle-2, 1);

        return ret;
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
        p.GPU_upload=nullptr;//std::bind(&gridUniforms::onGPUUpload, std::ref(*this));
        return p;
    };
    
    glm::vec2* trans;
    float* scale;
};

#endif //UNIFORM_IMPL_H