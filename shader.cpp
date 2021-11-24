#include "shader.h"


void shaderManager::bindUniformBlock(const std::string shader, const GLuint bindingPoint, const char* uniformName)
{
	unsigned int index=glGetUniformBlockIndex(programs[shader], uniformName);
	glUniformBlockBinding(programs[shader], index, bindingPoint);
}




void shaderManager::checkError(GLuint source, std::string type)
{
	GLint success;
	GLchar infoLog[1024];

	if (type != "PROGRAM")
	{
		glGetShaderiv(source, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(source, 1024, NULL, infoLog);
			std::cout << "shader compile ERROR! type: " << type
				<< " gl Info log: " << std::endl << infoLog
				<< std::endl;
		}
	}
	else
	{
		glGetProgramiv(source, GL_LINK_STATUS, &success);
		if (!success)
		{
			glGetProgramInfoLog(source, 1024, NULL, infoLog);
			std::cout << "shaderprogram link ERROR! type: " << type
				<< std::endl << "info log: " << std::endl << infoLog << std::endl;
		}
	}
}

void shaderManager::makeProgram(const char* name, const GLchar* vertShader, const GLchar* fragmentShader, const GLchar* geometryShader)
{
	GLuint id=this->compile(vertShader, fragmentShader, geometryShader);
	if(!glIsProgram(id))
	{
		DEBUGMSG("\nshadercompilation returend not-a-program");
		return;
	}
	programs[name]=id;

}

GLuint shaderManager::compile(const GLchar* vertShader, const GLchar* fragmentShader, const GLchar* geometryShader)
{
	GLuint vertex, fragment, geometry, progID;

	vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex, 1, &vertShader, NULL);
	glCompileShader(vertex);
	checkError(vertex, "VERTEX");

	fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment, 1, &fragmentShader, NULL);
	glCompileShader(fragment);
	checkError(vertex, "FRAGMENT");

	if (geometryShader != nullptr)
	{
		geometry = glCreateShader(GL_GEOMETRY_SHADER);
		glShaderSource(geometry, 1, &geometryShader, NULL);
		glCompileShader(geometry);
		checkError(vertex, "GEOMETRY");
	}


	progID = glCreateProgram();
	glAttachShader(progID, vertex);
	glAttachShader(progID, fragment);
	if (geometryShader != nullptr)
	{
		glAttachShader(progID, geometry);
	}
	
	glLinkProgram(progID);
	checkError(progID, "PROGRAM");

	glDeleteShader(vertex);
	glDeleteShader(fragment);
	if (geometryShader != nullptr)
	{
		glDeleteShader(geometry);
	}

	return progID;
}

