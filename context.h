#ifndef CONTEXT_H
#define CONTEXT_H
#include <stdio.h>
#include <iostream>
#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <string>

void GLAPIENTRY glDebugOutput(GLenum source,
GLenum type,
	unsigned int id,
	GLenum severity,
	GLsizei length,
	const char* message,
	const void* userParam);

struct renderContext
{

    static renderContext* getInstance()
    {
        static renderContext* instance= new renderContext;
        return instance;
    }

    void setWindowDim(const int width, const int height)
    {
        this->win_width=width;
        this->win_height=height;
        this->aspect_ratio=((float)width)/height;
    }
    bool makeWindowAndContext();

    SDL_Window* const getCWindow()const
    {
        return this->window;
    }

    std::string title="myWindow";
    int win_width=1600, win_height=960;
    float aspect_ratio=((float)win_height)/win_width;

private:
    SDL_GLContext context;
    int gl_context_maj=4, gl_context_min=2;
    SDL_Window *window;
    renderContext()=default;
};

	

bool renderContext::makeWindowAndContext()
{

    if(SDL_Init(SDL_INIT_VIDEO)<0)
    {
        printf("\nSDL_init failed");
        return false;
    }  

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, this->gl_context_maj);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, this->gl_context_min);   

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    window= SDL_CreateWindow(this->title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, this->win_width, this->win_height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if(!window)
    {
        printf("SDL WINDOW CREATIONE FAILED!");
        return false;
    }

    const char* error=SDL_GetError();
    if(*error!='\0')
    {
        printf("\nSDL_Error: %s\n", error);
        SDL_ClearError();
    }
    
    context= SDL_GL_CreateContext(window);
    const char* errorgl=SDL_GetError();
    if(*errorgl!='\0')
    {
        printf("\nSDL_Error: %s\n", errorgl);
        SDL_ClearError();
    }

    GLenum err = glewInit();
    
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(glDebugOutput, 0);

    return true;
}





void GLAPIENTRY glDebugOutput(GLenum source,
GLenum type,
	unsigned int id,
	GLenum severity,
	GLsizei length,
	const char* message,
	const void* userParam)
{
	// ignore non-significant error/warning codes
	if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;
	std::cout << "---------------" << std::endl;
	std::cout << "Debug message (" << id << "): " << message << std::endl;
	switch (source)
	{
	case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
	case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
	case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
	} std::cout << std::endl;
	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
	case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
	case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
	case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
	case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
	case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
	case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
	} std::cout << std::endl;
	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
	case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
	case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
	case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
	} std::cout << std::endl;
	std::cout << std::endl;
}

#endif //CONTEXT_H
