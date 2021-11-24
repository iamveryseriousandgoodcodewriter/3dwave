#ifndef MESH_H
#define MESH_H
#include <unordered_map>
#include <stdlib.h>
#include <string.h>
#include "utils.h"

struct mesh{

    size_t getVertexCount()const
    {
        return size/perVertexCount;
    }

    float* data=nullptr;
    int perVertexCount; //per vertex floast cnt
    size_t size;//size of the data array (in floats)
};

inline
mesh allocateMeshPos()
{
    
    float corners[]={
        

        //triangle -x -y -z
        0.0f,0.0f,-1.0f,
        0.0f,-1.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,

        //triangle x -y -z
        0.0f,0.0f,-1.0f,
        0.0f,-1.0f, 0.0f,
        1.0f, 0.0f, 0.0f,

        //triangle -x y -z
        0.0f,0.0f,-1.0f,
        0.0f,1.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,

        //triangle x y -z
        0.0f,0.0f,-1.0f,
        0.0f,1.0f, 0.0f,
        1.0f, 0.0f, 0.0f,

        //triangle -x -y z
        0.0f,0.0f,1.0f,
        0.0f,-1.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,

        //triangle x -y z
        0.0f,0.0f,1.0f,
        0.0f,-1.0f, 0.0f,
        1.0f, 0.0f, 0.0f,

        //triangle -x y z
        0.0f,0.0f,1.0f,
        0.0f,1.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,

        //triangle x y z
        0.0f,0.0f,1.0f,
        0.0f,1.0f, 0.0f,
        1.0f, 0.0f, 0.0f,

    };

    mesh m;
    m.size=8*9;
    m.perVertexCount=3;
    m.data=(float*) malloc(m.size*sizeof(float));
    if(!m.data)
    {
        DEBUGMSG("allocatemeshpos malloc failed");
        return m;
    }
    memcpy(m.data, corners, m.size*sizeof(float));
    return m;    
}

inline
mesh allocateMeshColor()
{
    float colors[]=
    {
        //first tri
        1.0f, 0.0f, 0.0f, 1.0f,
        0.5f, 0.0f, 0.0f, 1.0f,
        0.25f, 0.0f, 0.0f, 1.0f,

        .0f, 1.0f, 0.0f, 1.0f,
        0.0f, 0.5f, 0.0f, 1.0f,
        0.0f, 0.25f, 0.0f, 1.0f,

        .0f, 0.0f, 1.0f, 1.0f,
        0.0f, 0.0f, 0.5f, 1.0f,
        0.0f, 0.0f, 0.25f, 1.0f,

        1.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f, 1.0f,

        .75f, 0.25f, 0.0f, 1.0f,
        0.5f, 0.5f, 0.0f, 1.0f,
        0.25f, 0.75f, 0.0f, 1.0f,

        .0f, 0.75f, 0.25f, 1.0f,
        0.0f, 0.5f, 0.5f, 1.0f,
        0.0f, 0.25f, 0.75f, 1.0f,

        .75f, 0.0f, 0.25f, 1.0f,
        0.5f, 0.0f, 0.5f, 1.0f,
        0.25f, 0.0f, 0.75f, 1.0f,

        .25f, 0.25f, 0.0f, 1.0f,
        0.0f, 0.25f, 0.25f, 1.0f,
        0.25f, 0.0f, 0.25f, 1.0f,

    };

    mesh m;
    m.size=8*3*4;
    m.perVertexCount=4;
    m.data=(float*) malloc(m.size*sizeof(float));
    if(!m.data)
    {
        DEBUGMSG("\nallocatemeshpos malloc failed");
        return m;
    }
    memcpy(m.data, colors, m.size*sizeof(float));
    return m;

}

inline
mesh allocateSubGridMesh(int density)
{
    //min density of 2
    float step=(float)1/(density-1);
    std::vector<float> v_data;/*=
    {
        //bot left
        .5f, -.5f,
        .5f, .5f,
        -.5f, -.5f,
        //top right
        -.5f, .5f,
        .5f, .5f,
        -.5f, -.5f
    };*/
    
    
    for(int i=0;i<density-1;++i)//line
    {
        if(i%2)
        {
            for(int k=0;k<density;++k)//col
            {
                //first vertex
                v_data.push_back(-0.5f+step*k);
                v_data.push_back(0.5f-step*i);

                //second vertex
                v_data.push_back(-0.5f+step*k);
                v_data.push_back(0.5f-step*(i+1));
            }
        }
        else
        {
            for(int k=0;k<density;++k)//col
            {
                //first vertex
                v_data.push_back(0.5f-step*k);
                v_data.push_back(0.5f-step*i);

                //second vertex
                v_data.push_back(0.5f-step*k);
                v_data.push_back(0.5f-step*(i+1));
            }
        }
    }

    mesh m;
    m.size=v_data.size();
    m.perVertexCount=2;
    m.data=(float*)malloc(m.size*sizeof(float));
    if(!m.data)
    {
        DEBUGMSG("\nallocatesubgridmesh malloc failed");
        return m;
    }
    memcpy(m.data, &v_data[0], m.size*sizeof(float));
    return m;
}

struct meshContainer{

    static meshContainer* getInstance()
    {
        static meshContainer* instance = new meshContainer;
        return instance;
    }

    float* getData(const std::string name)
    {
        auto iter=data.find(name);
        if(iter==data.end())
            return nullptr;
        else
            return iter->second.data;
    }

    mesh getMesh(const std::string name)
    {
        auto iter=data.find(name);
        if(iter==data.end())
        {
            DEBUGMSG("\n meshcontainer get mesh failed, returning empty mesh");
            return mesh{nullptr, 0, 0};
        }
        return iter->second;
    }

    void addMesh(mesh _data, std::string name)
    {
        data.insert({name, _data});
    }

    void removeMesh(const std::string name)
    {
        auto iter=data.find(name);
        if(iter!=data.end())
        {
            data.erase(name);
        }

    }

    void freeAll()
    {
        for(auto it: data)
        {
            if(it.second.data)
            {
                free(it.second.data);
                it.second.data=nullptr;
            }
        }
    }

private:
    meshContainer()=default;
    std::unordered_map<std::string, mesh> data;
};

#endif //MESH_H