#include "GPUbuffer.h"

bool GPUbuffer::init(const GLenum drawType)
{
    this->drawtype=drawType;
    glGenBuffers(1, &this->bufferID);
    glBindBuffer(GL_ARRAY_BUFFER, this->bufferID);
    if(!glIsBuffer(this->bufferID))
    {
        #ifndef NDEBUG
        printf("\nGPU buffer init failed: glisbuffer returned false");
        #endif //NDEBUG
        return false;
    }
    glBufferData(GL_ARRAY_BUFFER, this->size*sizeof(float), NULL, drawType);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return true;
}

int GPUbuffer::getSameSubBuffer(const int ID)
{
    for(auto it: subBuffers)
    {
        if(it.ID==ID)
        {
            it.refCount++;
            return ID;
        }
    }
    return -1;
}

int GPUbuffer::newSubBuffer(const size_t n_size)
{
    int n_offset=this->getOffsetForSize(n_size);
    if(n_offset<0)return n_offset;

    subBuffer s;
    s.offset=n_offset;
    s.size=n_size;
    s.ID=++IDcounter;
    subBuffers.push_back(s);
    return s.ID;
}
subBuffer& GPUbuffer::getSubBuffer(const int ID)
{
    for(subBuffer& it: this->subBuffers)
    {
        if(it.ID==ID)
        {
            return it;
        }
    }
    DEBUGMSG("\n getSubBUffer ID not found, returning first entry");
    return *this->subBuffers.begin();
}

//warning: this is expensive
int GPUbuffer::getOffsetForSize(const size_t n_size)//in floats
{
    //some error checking and size confirmation
    if(n_size>this->size)
    {DEBUGMSG("\n GPU buffer too small to fit new SubBuffer");return -1;}

    size_t winner=0;
    for(auto& it: subBuffers){winner+=it.size;}
    if(winner+n_size>this->size)
    {DEBUGMSG("\n GPU buffer too full to fit new SubBuffer");return -2;}

    //sort so the algo works and we have a neat vector
    this->sortSubBuffers();
    //edge case where
    if(subBuffers.empty())
    {
        return 0;
    }
    //find a chunk in the buffer that fits size floats
    //also pack them neatly
    auto a=subBuffers.cbegin();
    auto b=subBuffers.cbegin()+1;
    auto witer=subBuffers.cend();
    
    winner=size;
    while(b!=subBuffers.cend())
    {
        if( (b->offset-(a->offset+a->size)) >= n_size )
        {
            if( (b->offset-(a->offset+a->size)) < winner )
            {
                winner=b->offset-(a->offset+a->size);
                witer=a;
            }
        }
        ++b, ++a;
    }
    if(witer!=subBuffers.cend())
    {
        return witer->offset+witer->size;
    }
    --b;//because b is nown at the end!
    if( (b->offset+b->size+n_size) <= size )
    {
        return b->offset+b->size;
    }
    
    DEBUGMSG("\n GPUbuffer::getOffsetForSize failed, returning id -3\n ->this s that the inbetween spaces are too small, and there is not enough e left at the end");
    
    return -3;
}
void GPUbuffer::init_subBuffer(const int ID, const int8_t iperVertexSize, const int8_t istride, float* data, const int data_size)
{
    subBuffer& subBuf=getSubBuffer(ID); 
    subBuf.setPerVertexSize(iperVertexSize);
    subBuf.setStride(istride);
    if(data&&data_size)
    {
        writeToSubBuffer(ID, data_size, data);
    }
}
void GPUbuffer::writeToSubBuffer(const int ID, const int w_size, const float* data, const int offset)
{
    subBuffer& subBuf=getSubBuffer(ID);
    if(w_size>subBuf.size)
    {DEBUGMSG("\n writeToSubBUffer failed: writeSize>buffersize");return;}
    glBindBuffer(GL_ARRAY_BUFFER, bufferID);
    float* buf=(float*)glMapBufferRange(GL_ARRAY_BUFFER, sizeof(float)*(subBuf.offset+offset), sizeof(float)*w_size, GL_MAP_WRITE_BIT);
    if(!buf)
    {
        DEBUGMSG("\n GPU_BUFFER writetosubbuffer failed: mapping returned NULL");
        return;
    }
    memcpy(buf, data, w_size*sizeof(float));
    glUnmapBuffer(GL_ARRAY_BUFFER);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
void GPUbuffer::sortSubBuffers()
{
    if(subBuffers.empty())return;
    auto cmp=[](subBuffer& lhs, subBuffer& rhs){
        return lhs.offset<rhs.offset;
    };
    std::sort(subBuffers.begin(), subBuffers.end(), cmp);
}
//TODO()
//write a function to repack the buffer in the GPU
void GPUbuffer::freeSubBuffer(const int ID)
{
    auto iter=subBuffers.begin();
    for(;iter!=subBuffers.end();++iter)
    {
        if(iter->ID==ID)
        {
            iter->refCount-=1;
            break;
        }
    }
    if(iter!=subBuffers.end()&&iter->refCount==0)
        subBuffers.erase(iter);
    else
        DEBUGMSG("\n freeSubBuffer failed: id not found");
}

void GPUbuffer::deleteBuffer()
{
    glDeleteBuffers(1, &this->bufferID);
    this->bufferID=0;
    this->drawtype=GL_FALSE;
}

/*
================================================================================
================================================================================
================================================================================
==================================LIST==========================================
================================================================================
================================================================================
================================================================================
================================================================================
*/


//you give size-in-type and you get a subBufferHandle
//warning: {-1, NULL} will be returned if allocation fails, so jot that down
subBufferHandle gpuBufferList::newSubBuffer(const size_t n_size)
{
    //opt: make it so we dont actually probe the buffers that are too small
    while(next_bufferSize<n_size){next_bufferSize*=2;}
    if(!head)
    {
        return allocateNodeAndAddSubBuffer(head, n_size);
    }
    
    gpuBufferList_node* curr=head;
    int ID=-1;
    while((curr=curr->next)->next)
    {
        if((ID=curr->buffer.newSubBuffer(n_size))>=0)
        {
            //yay :)
            //return the handle?
            return subBufferHandle(ID, &curr->buffer);
            
        }
        
    }
    if((ID=curr->buffer.newSubBuffer(n_size))>=0)
    {
        return subBufferHandle(ID, &curr->buffer);
    }
    if(ID<0)
    {
        while(curr->next){curr=curr->next;}
        return allocateNodeAndAddSubBuffer(curr->next, n_size);
    }

    DEBUGMSG("\n gpuBUfferLIst::newSubBuffer returning bad_handle. this should not happen");
    return subBufferHandle(-1, nullptr);
}
subBufferHandle gpuBufferList::allocateNodeAndAddSubBuffer(gpuBufferList_node* ptr, const size_t size)
{
    int ID=-1;
    try{
            ptr=allocateNewNode(next_bufferSize, type, drawType);
            if((ID=ptr->buffer.newSubBuffer(size))<0)
            {
                throw 30;
            }
            return subBufferHandle(ID, &ptr->buffer);
        }
        catch(const int& err)
        {
            DEBUGMSG("\n GPUBUFFERLIST::newSubBuffer exe caught, bullshit-buffer rned, msg:");
            switch(err)
            {
                case 10: DEBUGMSG("\nGPUbuffer bad alloc");break;
                case 20: DEBUGMSG("\nnew GPU-buffer init failed");break;
                case 30: DEBUGMSG("\n new subbuffer failed after new allocation, big error here");break;
                case 40: DEBUGMSG("\n max gpuBufferList-size of 1GB reached, allocation aborted");break;
                default: DEBUGMSG("\n this should not have happend, call the cops!");break;
            }
        }
    return subBufferHandle(-1, nullptr);
}
gpuBufferList_node* gpuBufferList::allocateNewNode(const size_t size, GLenum type, GLenum drawType)
{
    if(combined_size>=MAX_BUFFERSIZE_SUM)
    {
        throw 40;
    }
    combined_size+=size;
    gpuBufferList_node* latest=new gpuBufferList_node
    (GPUbuffer(size, type));
    if(!latest)
    {
        throw 10;
    }
    if(!latest->buffer.init(drawType))
    {
        delete latest;
        throw 20;
    }
    return latest;
}

void gpuBufferList::deleteNodes(gpuBufferList_node* node)
{

    if(node->next)
    {
        deleteNodes(node->next);
    }
    node->buffer.deleteBuffer();
    delete node;
    return;

}




/*maybe optimize later for containers bigger than a cache
void rawsort()
{
    int i=0, k;
    int gather[8];
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
        
        //call sort on args
        //or if cache optimized we ned to log the sort and return it
        
        i+=8;
    }
    //look at the last 8
    for(;k<writeIndex;++k)
    {
        if(entityIDs[k]!=-1)
        {
            entityIDs[i++]=entityIDs[k];
        }
    }
    //set write index
    writeIndex=i;
   // printf("\n sort done");
}*/