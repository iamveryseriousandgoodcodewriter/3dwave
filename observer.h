#ifndef OBSERVER_H
#define OBSERVER_H

#include "context.h"
#include "math/glm/glm.hpp"
#include "math/glm/ext/matrix_transform.hpp"
#include "math/glm/ext/matrix_clip_space.hpp"


#define PI 3.14159265359f

struct observer{

    static observer* getInstance()
    {
        static observer* instance = new observer;
        return instance;
    }

    void setupUBO()
    {
        glGenBuffers(1, &UBO);
        glBindBuffer(GL_UNIFORM_BUFFER, UBO);
        glBufferData(GL_UNIFORM_BUFFER, 2*sizeof(glm::mat4), NULL, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        glBindBufferBase(GL_UNIFORM_BUFFER, 0, UBO);
    }

    void updateUBO()const
    {
        glBindBuffer(GL_UNIFORM_BUFFER, UBO);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), &Projection);
        glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), &View);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    void init()
    {
        position =glm::vec3(-10.0f, .0f, 200.0f);
        direction=glm::vec3(1.0f, 0.0f, .0f);

        this->View=glm::lookAt(
            position, 
            position+direction, 
            glm::vec3(0.0f,0.0f,1.0f)
        );
        this->Projection=glm::perspective(
            (float)glm::radians(60.0f),
            renderContext::getInstance()->aspect_ratio,
            0.1f,
            1000.0f
        );
        SDL_SetRelativeMouseMode(SDL_TRUE);
        mouse_y=renderContext::getInstance()->win_height/2;
        mouse_x=renderContext::getInstance()->win_width/2;
        xi=PI/2;
        phi=0;
        //this->Projection=glm::mat4(1.0f);
        //this->View=glm::mat4(1.0f);
        this->setupUBO();
        this->updateUBO();
    }

    
    void update(const float dt)
    {
        direction=glm::vec3(cos(phi), sin(phi), cos(xi));   
        //if(this->v<this->v_max)
        //   this->v+=this->a*dt;

        this->position+=this->direction*this->v*dt;

        this->View=glm::lookAt(
            position, 
            direction+position, 
            glm::vec3(0.0f,0.0f,1.0f)
        );
        this->updateUBO();
    }
    void input(const SDL_Event& e)
    {
        if(e.type==SDL_MOUSEMOTION)
        {
            this->updateDiretionAngles(e.motion.xrel, e.motion.yrel);
        }
        else if(e.type==SDL_KEYDOWN)
        {
            if(e.key.keysym.sym==SDLK_w)
            {
                v=v_max;
            }
            else if(e.key.keysym.sym==SDLK_s)
            {
                v=-v_max;
            }
            else if(e.key.keysym.sym==SDLK_m)
            {
                mouse_sens+=0.1f;
            }
            else if(e.key.keysym.sym==SDLK_n)
            {
                mouse_sens-=0.1f;
            }
            else if(e.key.keysym.sym==SDLK_p)
            {
                printf("\npos: x%f,y%f,z%f", position.x, position.y, position.z);
            }
            else if(e.key.keysym.sym==SDLK_UP)
            {
                spike-=0.1f;
            }
            else if(e.key.keysym.sym==SDLK_DOWN)
            {
                spike+=0.1f;
            }
            else if(e.key.keysym.sym==SDLK_RIGHT)
            {
                damp+=0.1f;
            }
            else if(e.key.keysym.sym==SDLK_LEFT)
            {
                damp-=0.1f;
            }
        }
        else if(e.type==SDL_KEYUP)
        {
            if(e.key.keysym.sym==SDLK_w)
            {
                v=0;
            }
            if(e.key.keysym.sym==SDLK_s)
            {
                v=0;
            }
        }
    }
    void resetPosition()
    {
        this->position=glm::vec3(0.0f,0.0f,0.0f);
        this->a=0;
    }

    void updateDiretionAngles(int x, int y)
    {
        float xf=(float)-x/renderContext::getInstance()->win_width;
        float yf=(float)y/renderContext::getInstance()->win_height;

        //delta angles
        //xf*=FoV;
        //yf*=FoV;

        phi+=xf*mouse_sens;
        if(phi>2*PI)
            phi=0;
        else if(phi<0)
            phi=2*PI;

        xi+=yf*mouse_sens;
        if(xi>(PI))
            xi=PI;
        else if(xi<0)
            xi=0;
    }

    //put the camera stuff here
    GLuint UBO;
    glm::mat4 Projection;
    glm::mat4 View;


    //glm::vec3 scale;//these tell me how "far away" i am

    glm::vec3 position;//these tell me where im standing
    float a_max=0.01f;
    float a=0;//need angle vel for looking around
    float v_max=3.5f;
    float v=0;

    glm::vec3 direction;//these tell me where im looking
    float phi=0, xi=0;//phi ist längengrad, xi breitengrad
    int mouse_x, mouse_y;
    float mouse_sens=1.0f;

    //these build the appropriate mats
    float FoV=(float)glm::radians(45.0f);
    //put actual values here


    float damp=5.0f;
    float spike=2.0005f;

private:
    observer()=default;
};




#endif //OBSERVSER_H