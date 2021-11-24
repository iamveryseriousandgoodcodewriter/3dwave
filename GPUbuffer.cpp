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

//warning: this is expensive
int GPUbuffer::newSubBuffer(const size_t n_size)//in floats
{
    //some error checking and size confirmation
    if(n_size>this->size)
    {
        #ifndef NDEBUG
        printf("\n GPU buffer too small to fit new SubBuffer");
        #endif //NDEBUG
        return -1;
    }
    size_t winner=0;
    for(auto& it: subBuffers)
    {
        winner+=it.size;
    }
    if(winner+n_size>this->size)
    {
        #ifndef NDEBUG
        printf("\n GPU buffer too full to fit new SubBuffer");
        #endif //NDEBUG
        return -1;
    }
    //sort so the algo works and we have a neat vector
    this->sortSubBuffers();

    //edge case where
    if(subBuffers.empty())
    {
        subBuffer s{IDcounter++,1, n_size, 0};
        subBuffers.push_back(s);
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
        if(b->offset-(a->offset+a->size)>n_size)
        {
            if(b->offset-(a->offset+a->size)<winner)
            {
                winner=b->offset-(a->offset+a->size);
                witer=a;
            }
        }
        ++b, ++a;
    }
    auto addFunc=[n_size, this](std::vector<subBuffer>::const_iterator a){
        subBuffer s;
        s.offset=a->offset+a->size;
        s.refCount=1;
        s.size=n_size;
        s.ID=IDcounter++;
        subBuffers.push_back(s);
        
        printf("\n adding ID=%i", s.ID);
        return s.ID;
    };
    if(witer!=subBuffers.cend())
    {
        return addFunc(witer);
    }
    --b;//because b is nown at the end!
    if(b->offset+b->size+n_size<size)
    {
        return addFunc(b);
    }
    #ifndef NDEBUG
    printf("\n newSubBuffer allocation failed, returning id -1\n ->this s that the inbetween spaces are too small, and there is not enough e left at the end");
    #endif //NDEBUG
    return -1;
}
void GPUbuffer::writeToSubBuffer(const int ID, int offset, int w_size, float* data)
{
    glBindBuffer(GL_ARRAY_BUFFER, bufferID);
    float* buf=(float*)glMapBufferRange(GL_ARRAY_BUFFER, sizeof(float)*(subBuffers[ID].offset+offset), sizeof(float)*w_size, GL_MAP_WRITE_BIT);
    if(!buf)
    {
        #ifndef NDEBUG
        printf("\n GPU_BUFFER writetosubbuffer failed");
        #endif
        return;
    }
    memcpy(buf, data, w_size*sizeof(float));
    glUnmapBuffer(GL_ARRAY_BUFFER);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
void GPUbuffer::sortSubBuffers()
{
    if(subBuffers.empty())return;
    auto cmp=[](subBuffer lhs, subBuffer rhs){
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
    #ifndef NDEBUG
    else
        printf("\n freeSubBuffer on ID=%i failed: id not found", ID);
    #endif //NDEBUG
}

/*
================================================================================
================================================================================
================================================================================
==========================UNIFORM CONTINER======================================
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
==================================DRAWABLE======================================
================================================================================
================================================================================
================================================================================
================================================================================
*/

void drawable::staticBufferSetup(std::vector<std::string> meshes)
{
    if(staticBuffer->drawtype==GL_FALSE)
    {
        DEBUGMSG("\nstatic buffer setup failed: GPUbuffer uninitialized");
        return;
    }
    int i=0;
    if(!glIsVertexArray(VAO))
        glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    for(auto it : meshes)
    {
        
        //populate gpu buffer with mesh data
        mesh _mesh=meshContainer::getInstance()->getMesh(it);
        int subBufferID=staticBuffer->newSubBuffer(_mesh.size);
        if(subBufferID==-1)
        {
            #ifndef NDEBUG
            printf("\n vertexDataSetup failed: subBUfferID is -1");
            #endif //NDEBUG
            return;
        }
        this->staticBuffer->writeToSubBuffer(subBufferID, 0, _mesh.size, _mesh.data);
        this->staticSubBuffers.insert({it,subBufferID});
        //set up vao to read the afore mentioned data
        glBindBuffer(GL_ARRAY_BUFFER, staticBuffer->bufferID);
        glVertexAttribPointer(i,
         _mesh.perVertexCount,//vertex-vector size
          GL_FLOAT, GL_FALSE,//type+normalized
          0,//stride between veretx-vectors (for interleaving)
          (void*)(staticBuffer->subBuffers[subBufferID].offset*sizeof(float))//et
        );
        glEnableVertexAttribArray(i++);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    glBindVertexArray(0); 
}


void drawable::oneDynamicBufferSetup(const std::string name,const size_t sizeInFloats, const int perVertSize, const int attribPtrCnt, const int locationBegin, const int attribDivisor)
{
    if(!dynamicBuffer)
    {
        DEBUGMSG("\n trying to write to non-existant dynmic buffer in drawable::dynamic buffersetup");
        return;
    }
    if(dynamicBuffer->drawtype==GL_FALSE)
    {
        DEBUGMSG("\nstatic buffer setup failed: GPUbuffer uninitialized");
        return;
    }
    int sBufID=this->dynamicBuffer->newSubBuffer(sizeInFloats);
    this->dynamicSubBuffers.insert({name, sBufID});
    
    size_t offset=perVertSize*sizeof(float);
    size_t globalBufferOffset=this->dynamicBuffer->subBuffers[sBufID].offset*sizeof(float);
    size_t stride=attribPtrCnt*offset;
    if(!glIsVertexArray(VAO))
        glGenVertexArrays(1, &VAO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, this->dynamicBuffer->bufferID);

    for(int i=0;i<attribPtrCnt;++i)
    {
        glEnableVertexAttribArray(locationBegin+i);
        glVertexAttribPointer(locationBegin+i, 
        perVertSize, 
        GL_FLOAT, 
        GL_FALSE, 
        stride, 
        (void*)(offset*i+globalBufferOffset));
        glVertexAttribDivisor(locationBegin+i, attribDivisor);
    }
    glBindVertexArray(0);
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