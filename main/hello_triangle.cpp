#include <fstream>
#include <iostream>
#include <sstream>
#include <GL/glew.h>

#include "engine.h"
#include "file_utility.h"
#include "scene.h"

namespace gpr5300
{
    class HelloTriangle final : public Scene
    {
    public:
        void Begin() override;
        void End() override;
        void Update(float dt) override;
    private:
        GLuint vertexShader_ = 0;
        GLuint fragmentShader_ = 0;
        GLuint program_ = 0;
        GLuint vao_ = 0;
    };

    void HelloTriangle::Begin()
    {
        //Load shaders
        const auto vertexContent = LoadFile("data/shaders/hello_triangle/triangle.vert");
        const auto* ptr = vertexContent.data();
        vertexShader_ = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader_, 1, &ptr, nullptr);
        glCompileShader(vertexShader_);
        //Check success status of shader compilation
        GLint success;
        glGetShaderiv(vertexShader_, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            std::cerr << "Error while loading vertex shader\n";
        }
        const auto fragmentContent = LoadFile("data/shaders/hello_triangle/triangle.frag");
        ptr = fragmentContent.data();
        fragmentShader_ = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader_, 1, &ptr, nullptr);
        glCompileShader(fragmentShader_);
        //Check success status of shader compilation

        glGetShaderiv(fragmentShader_, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            std::cerr << "Error while loading fragment shader\n";
        }
        //Load program/pipeline
        program_ = glCreateProgram();
        glAttachShader(program_, vertexShader_);
        glAttachShader(program_, fragmentShader_);
        glLinkProgram(program_);
        //Check if shader program was linked correctly

        glGetProgramiv(program_, GL_LINK_STATUS, &success);
        if (!success)
        {
            std::cerr << "Error while linking shader program\n";
        }

        //Empty vao
        glCreateVertexArrays(1, &vao_);
    }

    void HelloTriangle::End()
    {
        //Unload program/pipeline
        glDeleteProgram(program_);

        glDeleteShader(vertexShader_);
        glDeleteShader(fragmentShader_);

        glDeleteVertexArrays(1, &vao_);

    }

    void HelloTriangle::Update(float dt)
    {
        //Draw program
        glUseProgram(program_);
        glBindVertexArray(vao_);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
}
int main(int argc, char** argv)
{
    gpr5300::HelloTriangle scene;
    gpr5300::Engine engine(&scene);
    engine.Run();

    return EXIT_SUCCESS;
}