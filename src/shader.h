#ifndef SHADER_H  
#define SHADER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader 
{
public:
	//ID program object
	GLuint shaderProgramID;
	
	//Constructor reads shader file from disk and creates it
	Shader(const GLchar* vertexPath, const GLchar* fragmentPath, const GLchar* geometryPath = nullptr)
	{
        //load source code from file vertex shader and fragment shader
        std::string vertexCode;
        std::string fragmentCode;
        std::string geometryCode;
        std::ifstream vertexShaderFile;
        std::ifstream fragmentShaderFile;
        std::ifstream geometryShaderFile;

        vertexShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fragmentShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        geometryShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try {
            vertexShaderFile.open(vertexPath);
            fragmentShaderFile.open(fragmentPath);
            std::stringstream vertexShaderStream, fragmentShaderStream;

            vertexShaderStream << vertexShaderFile.rdbuf();
            fragmentShaderStream << fragmentShaderFile.rdbuf();

            vertexShaderFile.close();
            fragmentShaderFile.close();

            vertexCode = vertexShaderStream.str();
            fragmentCode = fragmentShaderStream.str();
            // if geometry shader path is present, also load a geometry shader
            if (geometryPath != nullptr)
            {
                geometryShaderFile.open(geometryPath);
                std::stringstream geometryShaderStream;
                geometryShaderStream << geometryShaderFile.rdbuf();
                geometryShaderFile.close();
                geometryCode = geometryShaderStream.str();
            }

            spdlog::info("Load file");
        }
        catch (std::ifstream::failure& e)
        {
            spdlog::error("ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: {}", e.what());
        }

        //source code
        const char* vertexShaderCode = vertexCode.c_str();
        const char* fragmentShaderCode = fragmentCode.c_str();

        //compile shaders
         
        //id to vertex shader and set type like GL_VERTEX_SHADER
        GLuint vertexShader;
        vertexShader = glCreateShader(GL_VERTEX_SHADER);
        //compile vertex shader
        glShaderSource(vertexShader, 1, &vertexShaderCode, NULL);
        glCompileShader(vertexShader);
        checkCompileShaderAndLinkErrors(vertexShader, "VERTEX");

        //id to fragment shader and set type like GL_FRAGMENT_SHADER
        GLuint fragmentShader;
        fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        //compile fragment shader
        glShaderSource(fragmentShader, 1, &fragmentShaderCode, NULL);
        glCompileShader(fragmentShader);
        checkCompileShaderAndLinkErrors(fragmentShader, "FRAGMENT");

        GLuint geometry;
        if (geometryPath != nullptr)
        {
            const char* geometryShaderCode = geometryCode.c_str();
            geometry = glCreateShader(GL_GEOMETRY_SHADER);
            glShaderSource(geometry, 1, &geometryShaderCode, NULL);
            glCompileShader(geometry);
            checkCompileShaderAndLinkErrors(geometry, "GEOMETRY");
        }

        //program object
        
        //create program object
        shaderProgramID = glCreateProgram();

        //attach and link shaders to program object
        glAttachShader(shaderProgramID, vertexShader);
        glAttachShader(shaderProgramID, fragmentShader);
        if (geometryPath != nullptr)
            glAttachShader(shaderProgramID, geometry);
        glLinkProgram(shaderProgramID);
        checkCompileShaderAndLinkErrors(shaderProgramID, "PROGRAM");


        //delete shaders after link them
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        if (geometryPath != nullptr)
            glDeleteShader(geometry);
	}

    //activate the shader
    void use()
    {
        glUseProgram(shaderProgramID);
    }

    //utility uniform functions
    void setBool(const std::string& name, bool value) const
    {
        glUniform1i(glGetUniformLocation(shaderProgramID, name.c_str()), (int)value);
    }

    void setInt(const std::string& name, int value) const
    {
        glUniform1i(glGetUniformLocation(shaderProgramID, name.c_str()), value);
    }

    void setFloat(const std::string& name, float value) const
    {
        glUniform1f(glGetUniformLocation(shaderProgramID, name.c_str()), value);
    }

    void setVec2(const std::string& name, const glm::vec2& value) const
    {
        glUniform2fv(glGetUniformLocation(shaderProgramID, name.c_str()), 1, &value[0]);
    }
    void setVec2(const std::string& name, float x, float y) const
    {
        glUniform2f(glGetUniformLocation(shaderProgramID, name.c_str()), x, y);
    }
    
    void setVec3(const std::string& name, const glm::vec3& value) const
    {
        glUniform3fv(glGetUniformLocation(shaderProgramID, name.c_str()), 1, &value[0]);
    }

    void setVec3(const std::string& name, float x, float y, float z) const
    {
        glUniform3f(glGetUniformLocation(shaderProgramID, name.c_str()), x, y, z);
    }
    
    void setVec4(const std::string& name, const glm::vec4& value) const
    {
        glUniform4fv(glGetUniformLocation(shaderProgramID, name.c_str()), 1, &value[0]);
    }

    void setVec4(const std::string& name, float x, float y, float z, float w) const
    {
        glUniform4f(glGetUniformLocation(shaderProgramID, name.c_str()), x, y, z, w);
    }
    
    void setMat2(const std::string& name, const glm::mat2& mat) const
    {
        glUniformMatrix2fv(glGetUniformLocation(shaderProgramID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

    void setMat3(const std::string& name, const glm::mat3& mat) const
    {
        glUniformMatrix3fv(glGetUniformLocation(shaderProgramID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

    void setMat4(const std::string& name, const glm::mat4& mat) const
    {
        glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

private:

    void checkCompileShaderAndLinkErrors(GLuint shader, std::string typeShader)
    {
        //check success shader compile and link
        GLint success;
        GLchar infoLog[1024];

        if (typeShader != "PROGRAM")
        {
            //check success shader compile
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

            if (!success)
            {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                spdlog::error("ERROR::SHADER_COMPILATION_ERROR of type: {}\n{}", typeShader, infoLog);
            }
            else
            {
                spdlog::info("Compile success shader");
            }
        }
        else
        {
            //check success link to program object
            glGetProgramiv(shader, GL_LINK_STATUS, &success);

            if (!success)
            {
                glGetProgramInfoLog(shader, 512, NULL, infoLog);
                spdlog::error("ERROR::SHADER::PROGRAM::LINK_FAILED\n{}", infoLog);
            }
            else
            {
                spdlog::info("Link success to program object");
            }
        }
    }

};

#endif