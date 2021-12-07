#ifndef UTILS_H
#define UTILS_H
#include <chrono>
#include <stdio.h>
#include <random>
#include <iostream>
#include <functional>



class timer{
public:
    void start()
    {
        this->begin=std::chrono::high_resolution_clock::now();
        this->delta_begin=std::chrono::high_resolution_clock::now();
    }
    void print(const char* msg)
    {
        const double sec=dur.count()*0.000000001;
        printf("\n%s time passed: %ldns or %fs", msg, this->dur.count(), sec);
    }
    void stop()
    {
        this->end=std::chrono::high_resolution_clock::now();
        this->dur=end-begin;
    }
    //returns tiome since start
    double getDuration()
    {
        this->stop();
        const double sec=dur.count()*0.000000001;
        return sec;
    }
    //returns sec since last call to this func
    double getDeltaDur()
    {
        this->end=std::chrono::high_resolution_clock::now();
        this->dur=end-delta_begin;
        this->delta_begin=end;
        const double sec=dur.count()*0.000000001;
        return sec;
    }
    
    void stoprint(const char* msg)
    {
        this->stop();
        this->print(msg);
    }
    void pause();
private:
    std::chrono::time_point<std::chrono::high_resolution_clock, std::chrono::nanoseconds> begin;
    std::chrono::time_point<std::chrono::high_resolution_clock, std::chrono::nanoseconds> delta_begin;
    std::chrono::time_point<std::chrono::high_resolution_clock, std::chrono::nanoseconds> end;
    std::chrono::duration<int64_t, std::nano> dur;
};

class rng{

public:
    static float getFloat()
    {
        std::uniform_real_distribution<float> distro(-1, 1);
        return distro(mt);
    }
    static float getFloat(float min, float max)
    {
        std::uniform_real_distribution<float> distro(min, max);
        return distro(mt);
    }
    //WARNING: MALLOCATES! USE GETFLOATVECTOR IF YOU CANT FREE
    static float* mallocFloatArray(const size_t size, const float min=-1, const float max=1)
    {
        float* arr=(float*)malloc(size*sizeof(float));
        if(!arr){printf("erroR: rng::getFloatArray malloc failed"); return nullptr;}
        for(int i=0;i<size;++i)
        {
            arr[i]=getFloat(min, max);
        }
        return arr;
    }
    //gen size floats between min,max in arr. make sure arr can hold at least size floats. use start start later
    static float* fillFloatArray(float* arr, const size_t size, const float min=-1, const float max=1, const int start=0)
    {
        for(size_t i=0;i<size;++i)
        {
            arr[i+start]=getFloat(min, max);
        }
        return arr;
    }

    static std::vector<float> getFloatVector(const size_t size, const float min=-1, const float max=1)
    {
        float* arr=mallocFloatArray(size, min, max);
        if(!arr){return std::vector<float>();}
        std::vector<float> v{arr, arr+size};
        free(arr);
        return v;
    }

    static int getInt(int min, int max)
    {
        std::uniform_int_distribution<int> dis(min, max);
        return dis(mt);
    }

    static std::vector<int> getIntVector(size_t size, int min, int max)
    {
        std::vector<int> v;
        for(size_t i=0; i<size;++i)
        {
            v.push_back(getInt(min, max));
        }
        return v;
    }
private:
    static std::random_device r;
    static std::mt19937_64 mt;
};


struct controlFlags32
{
    void set(const int flag)
    {
        this->flags|=flag;
    }
    void remove(const int flag)
    {
        this->flags&=~flag;
    }
    int is(const int flag)
    {
        return flags&flag;
    }


private:
    int flags=0;
};


struct bad_construction_exe
{
    enum : int{
        bad_gpu_subBufferHandle,
        bad_return,
        bad_alloc,
        bad_argCnt,
        bad_arg
    };
    bad_construction_exe(const int iID, const std::string imsg)
    :ID(iID), msg(imsg){}

    int ID;
    std::string msg;
};
struct terminal_exe
{
    enum : int{
        bad_alloc,
        bad_scene_construction
    };
    terminal_exe(const int iID, const std::string imsg)
    :ID(iID), msg(imsg){}
    int ID;
    std::string msg;
};

struct pipeline{
    virtual std::function<void(const float)> getPipe()=0;
};




#ifndef NDEBUG
#define DEBUGMSG(x) (std::cout<<(x))
#define PRINTWRAPPER(x) (printf(x))
#else
#define DEBUGMSG(x)
#define PRINTWRAPPER(x)
#endif


#endif //UTILS_H