#include "dynamicRenderData.h"
#include <string.h>
/*
================================================================================
================================================================================
================================================================================
======================================LIST======================================
================================================================================
================================================================================
================================================================================
================================================================================
*/


void uniformContainer_list::addContainer(
    const std::string name,
    uniformContainer* container
)
{
    this->containers.insert({name, container});
    this->unpackEntryTicket(container->makeEntryTicket());
}

void uniformContainer_list::unpackEntryTicket(pipelineFuncPtrs ticket)
{
    if(ticket.sort)
    {
        sort_stage.push_back(ticket.sort);
    }
    if(ticket.dt)
    {
        dt_update_stage.push_back(ticket.dt);
    }
    if(ticket.internal)
    {
        internal_update_stage.push_back(ticket.internal);
    }
    if(ticket.GPU_upload)
    {
        on_GPU_upload_stage.push_back(ticket.GPU_upload);
    }

}








/*
================================================================================
================================================================================
================================================================================
======================================BASE======================================
================================================================================
================================================================================
================================================================================
================================================================================
*/
int uniformContainer::add()
{
    if(writeIndex==entitySize-1)
    {
        #ifndef NDEBUG
        printf("\n uniform container full!");
        #endif //NDEBUG
        return -1;
    }
    return (entityIDs[writeIndex++]=IDCounter++);        
}
void uniformContainer::remove(const int ID)
{
    if(ID==-1)return;
    int i=0;
    while(entityIDs[i++]<ID);
    if(entityIDs[i-1]==ID)//some security cause oyu never know
        entityIDs[i-1]=-1;
}

void uniformContainer::sortWithData(std::function<void(int, int*)> subSorts)
{
    int i=0, k=0, j;
    int gather[8];
    int value[8];
    int gatherIndex=0;
    //__m256i intv8;
    while(k<writeIndex-8)
    {
        for(gatherIndex=0;gatherIndex<8;++k)
        {
            if(entityIDs[k]!=-1)
            {
                gather[gatherIndex++]=k;
            }
            
        }
        //weird ass shit going down here
        //intv8 = _mm256_i32gather_epi32(entityIDs, _mm256_loadu_si256(256i*)&gather[0]), 4);
        
        //_mm256_storeu_si256((__m256i*)(entityIDs+i), intv8);
        for(j=0;j<8;++j)
        {
            value[j]=entityIDs[gather[j]];
        }
        for(j=0;j<8;++j)
        {
            entityIDs[i+j]=value[j];
        }
        
        
        //call sort on args
        //or if cache optimized we ned to log the sort and return it
        subSorts(i, gather);
        
        
        i+=8;
    }
    //look at the last 8
    for(j=0;j<8;++j)
    {
        gather[j]=writeIndex;
    }
    for(gatherIndex=0;k<writeIndex;++k)
    {
        if(entityIDs[k]!=-1)
        {
            gather[gatherIndex++]=k;
        }
    }
    for(j=0;j<8;++j)
    {
        value[j]=entityIDs[gather[j]];
    }
    for(j=0;j<8;++j)
    {
        entityIDs[i+j]=value[j];
    }
    subSorts(i, gather);

    //set write index
    writeIndex=i+gatherIndex;
    //printf("\n sort done");
} 

/*
================================================================================
================================================================================
================================================================================
=====================================GRID======================================
================================================================================
================================================================================
================================================================================
================================================================================
*/

int gridUniforms::add(const glm::vec2 pos, const float iscale)
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
void gridUniforms::sort()
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


/*
================================================================================
================================================================================
================================================================================
==================================POLYGONS======================================
================================================================================
================================================================================
================================================================================
================================================================================
*/


void uniform3d::reAllocate(const size_t newSize)
{
    if(newSize<writeIndex)
    {
        DEBUGMSG("\n unfiromContainer reAlloc failed: trying to shrink w writeIndex");
        return;
    }
    trans=allocateAndCopy(newSize, trans, writeIndex);
    rot=allocateAndCopy(newSize, rot, writeIndex);
    scale=allocateAndCopy(newSize, scale, writeIndex);
    
}

int uniform3d::add()
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
int uniform3d::add(const glm::vec3 itrans, const glm::vec3 irot, const glm::vec3 iscale)
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
void uniform3d::sort()
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
void uniform3d::buildModelMats()
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