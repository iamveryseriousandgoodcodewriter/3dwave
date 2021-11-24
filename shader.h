#ifndef SHADER_H
#define SHADER_H
#include <map>
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <string>
#include <vector>
#include "utils.h"

struct shaderManager
{
    static shaderManager* getInstance()
    {
        static shaderManager* instance=new shaderManager;
        return instance;
    }

    void makeProgram(const char* name, const GLchar* vertShader, const GLchar* fragmentShader, const GLchar* geometryShader=nullptr);
    
    void checkError(GLuint source, std::string type);
    GLuint compile(const GLchar* vertShader, const GLchar* fragmentShader, const GLchar* geometryShader);

    void bindUniformBlock(const std::string shader, const GLuint bindingPoint, const char* uniformName);

    GLuint getShader(std::string name)
    {
        auto iter= programs.find(name);
        return iter->second;
    }

    

    std::map<std::string, GLuint> programs;
private:
    shaderManager()=default;
};


#endif //SHADER_H