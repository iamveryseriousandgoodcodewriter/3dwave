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
        DEBUGMSG("\n uniform container full!");
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


