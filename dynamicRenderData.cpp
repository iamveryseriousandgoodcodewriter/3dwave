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
uniformContainer::uniformContainer( const size_t isize, const int iuID)
:typeID(iuID), entitySize(isize+isize%8),  
entityIDs(new int[isize+isize%8]())
{
    for(size_t i=0;i<entitySize;++i)
    {
        entityIDs[i]=-1;
    }
}
//RULE OF 5
uniformContainer::uniformContainer(const uniformContainer& from)
:IDCounter(from.IDCounter), writeIndex(from.writeIndex), entitySize(from.entitySize),
deleteQue(from.deleteQue), typeID(from.typeID)
{
    entityIDs=new int[entitySize];
    memset(entityIDs, -1, entitySize*sizeof(int));
    memcpy(entityIDs, from.entityIDs, writeIndex*sizeof(int));
}
uniformContainer& uniformContainer::operator=(const uniformContainer& from)
{
    if(this==&from) return *this;
    if(from.writeIndex!=this->entitySize)
    {
        this->reAllocate(from.entitySize);
    }
    
    this->IDCounter=from.IDCounter;
    this->entitySize=from.entitySize;
    this->writeIndex=from.writeIndex;
    this->deleteQue=from.deleteQue;
    this->typeID=from.typeID;
    memcpy(this->entityIDs, from.entityIDs, this->entitySize);
    return *this;
}
uniformContainer::uniformContainer(uniformContainer&& from)noexcept
:IDCounter(from.IDCounter), writeIndex(from.writeIndex), entitySize(from.entitySize),
deleteQue(from.deleteQue), typeID(from.typeID)
{
    this->entityIDs=from.entityIDs;
    from.entityIDs=nullptr;
    from.entitySize=0;
}
uniformContainer& uniformContainer::operator=(uniformContainer&& from)noexcept
{
    if(this->entityIDs)delete[] this->entityIDs;
    this->IDCounter=from.IDCounter;
    this->entitySize=from.entitySize;
    this->writeIndex=from.writeIndex;
    this->deleteQue=std::move(from.deleteQue);
    this->typeID=from.typeID;
    
    from.entityIDs=nullptr;
    from.entitySize=0;
    return *this;
}





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
    this->deleteQue.push_back(ID);
    
}

void uniformContainer::reAllocate(const size_t newSize)
{
    int* newArray=new int[newSize+newSize%8];
    memset(newArray, -1, entitySize*sizeof(int));
    memcpy(newArray, entityIDs, writeIndex*sizeof(int));
    delete[] entityIDs;
    entityIDs=allocateAndCopy(newSize, entityIDs, writeIndex);
    entitySize=newSize;
}

//opt: look at how many get rmved compared to how many exist and choose an appr
//      algo, like when almost all get deleted it might be better to sort           deletions and then just remove all
void uniformContainer::onBulkRemoval()
{
    for(int& id : deleteQue)
    {
        int end=writeIndex-1, mid=writeIndex-1/2;
        while(id!=entityIDs[mid]&&end!=mid)
        {
            if(id<entityIDs[mid])
            {
                end=entityIDs[mid];
                entityIDs[mid]/=2;
            }
            else if(id>entityIDs[mid])
            {
                entityIDs[mid]+=(end-entityIDs[mid])/2;
            }
        }
        if(id!=entityIDs[mid])
        {
            id=writeIndex;
        }
        else{
            id=mid;
        }
    }
    for(int id:deleteQue)
    {
        entityIDs[id]=-1;
    }
    deleteQue.clear();
}

void uniformContainer::sortWithData(std::function<void(int, int*)> subSorts)
{
    if(deleteQue.empty())
    {
        return;
    }
    
    onBulkRemoval();

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
        //weird shit going down here
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
void uniformContainer::printIDs(const char* msg)const
{
    printf("\n%s", msg);
    for(int i=0; i<writeIndex; ++i)
    {
        printf("\nId: %i, vIndex:%i", entityIDs[i], i);
    }
} 


