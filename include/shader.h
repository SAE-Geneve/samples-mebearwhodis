#ifndef SHADER_H_
#define SHADER_H_

#include <iostream>
#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>

#include "file_utility.h"

class Shader
{
public:
    unsigned int id_;
    Shader() = default;
    //Constructor from paths
    Shader(const char* vertex_path, const char* fragment_path)
    {
        GLint success;
        //Load shaders
        const auto vertex_content = gpr5300::LoadFile(vertex_path);
        const auto* v_shader_code = vertex_content.data();
        const unsigned int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex_shader, 1, &v_shader_code, nullptr);
        glCompileShader(vertex_shader);
        glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            std::cerr << "Error while loading vertex shader\n";
        }

        const auto fragment_content = gpr5300::LoadFile(fragment_path);
        const auto* f_shader_code = fragment_content.data();
        const unsigned int fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment_shader, 1, &f_shader_code, nullptr);
        glCompileShader(fragment_shader);
        glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            std::cerr << "Error while loading fragment shader\n";
        }

        //Load program
        id_ = glCreateProgram();
        glAttachShader(id_, vertex_shader);
        glAttachShader(id_, fragment_shader);
        glLinkProgram(id_);
        //Check if shader program was linked correctly
        glGetProgramiv(id_, GL_LINK_STATUS, &success);
        if (!success)
        {
            std::cerr << "Error while linking shader program\n";
        }

        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
    }

    void Use() const
    {
        glUseProgram(id_);
    }

    void Delete() const
    {
        glDeleteProgram(id_);
    }

    //Uniform functions
    void SetBool(const std::string &name, const bool value) const
    {
        glUniform1i(glGetUniformLocation(id_, name.c_str()), static_cast<int>(value));
    }
    void SetInt(const std::string &name, const int value) const
    {
        glUniform1i(glGetUniformLocation(id_, name.c_str()), value);
    }
    void SetFloat(const std::string &name, const float value) const
    {
        glUniform1f(glGetUniformLocation(id_, name.c_str()), value);
    }
    void SetVec2(const std::string &name, const glm::vec2 &value) const
    {
        glUniform2fv(glGetUniformLocation(id_, name.c_str()), 1, &value[0]);
    }
    void SetVec2(const std::string &name, const float x, const float y) const
    {
        glUniform2f(glGetUniformLocation(id_, name.c_str()), x, y);
    }
    void SetVec3(const std::string &name, const glm::vec3 &value) const
    {
        glUniform3fv(glGetUniformLocation(id_, name.c_str()), 1, &value[0]);
    }
    void SetVec3(const std::string &name, const float x, const float y, const float z) const
    {
        glUniform3f(glGetUniformLocation(id_, name.c_str()), x, y, z);
    }
    void SetVec4(const std::string &name, const glm::vec4 &value) const
    {
        glUniform4fv(glGetUniformLocation(id_, name.c_str()), 1, &value[0]);
    }
    void SetVec4(const std::string &name, const float x, const float y, const float z, const float w) const
    {
        glUniform4f(glGetUniformLocation(id_, name.c_str()), x, y, z, w);
    }
    void SetMat2(const std::string &name, const glm::mat2 &value) const
    {
        glUniformMatrix2fv(glGetUniformLocation(id_, name.c_str()), 1,GL_FALSE, value_ptr(value));
    }
    void SetMat3(const std::string &name, const glm::mat3 &value) const
    {
        glUniformMatrix3fv(glGetUniformLocation(id_, name.c_str()), 1, GL_FALSE, value_ptr(value));
    }
    void SetMat4(const std::string &name, const glm::mat4 &value) const
    {
        glUniformMatrix4fv(glGetUniformLocation(id_, name.c_str()), 1, GL_FALSE, value_ptr(value));
    }
};

#endif //SHADER_H_
