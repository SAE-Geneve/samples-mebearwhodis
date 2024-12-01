#include <fstream>
#include <imgui.h>
#include <iostream>
#include <sstream>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "engine.h"
#include "file_utility.h"
#include "free_camera.h"
#include "scene.h"
#include "texture_loader.h"

namespace gpr5300
{
    class HelloLight final : public Scene
    {
    public:
        void Begin() override;
        void End() override;
        void Update(float dt) override;
        void OnEvent(const SDL_Event& event, const float dt) override;
        void DrawImGui() override;

    private:
        GLuint vertexShader_ = 0;
        GLuint fragmentShader_ = 0;
        GLuint light_vertexShader_ = 0;
        GLuint light_fragmentShader_ = 0;
        GLuint program_ = 0;
        GLuint light_program_ = 0;
        GLuint vao_ = 0;
        GLuint vbo_ = 0;
        GLuint ebo_ = 0;
        GLuint light_vao_ = 0;
        unsigned int texture_;

        float elapsedTime_ = 0.0f;

        glm::vec3 cubePositions[10] = {};
        glm::vec3 light_position_ = glm::vec3(1.2f, 1.0f, 2.0f);

        FreeCamera* camera_ = nullptr;
    };

    void HelloLight::Begin()
    {
        camera_ = new FreeCamera();
        TextureManager texture_manager;
        texture_ = texture_manager.CreateTexture("data/textures/container.jpg");


        //Main program
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

        //Light program
        //Load shaders
        const auto lightVertexContent = LoadFile("data/shaders/hello_light/light.vert");
        ptr = lightVertexContent.data();
        light_vertexShader_ = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(light_vertexShader_, 1, &ptr, nullptr);
        glCompileShader(light_vertexShader_);
        //Check success status of shader compilation
        glGetShaderiv(light_vertexShader_, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            std::cerr << "Error while loading vertex shader\n";
        }
        const auto lightFragmentContent = LoadFile("data/shaders/hello_light/light.frag");
        ptr = lightFragmentContent.data();
        light_fragmentShader_ = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(light_fragmentShader_, 1, &ptr, nullptr);
        glCompileShader(light_fragmentShader_);
        //Check success status of shader compilation

        glGetShaderiv(light_fragmentShader_, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            std::cerr << "Error while loading fragment shader\n";
        }
        //Load program/pipeline
        light_program_ = glCreateProgram();
        glAttachShader(light_program_, light_vertexShader_);
        glAttachShader(light_program_, light_fragmentShader_);
        glLinkProgram(light_program_);
        //Check if shader program was linked correctly

        glGetProgramiv(light_program_, GL_LINK_STATUS, &success);
        if (!success)
        {
            std::cerr << "Error while linking shader program\n";
        }

        // configure global opengl state
        // -----------------------------
        glEnable(GL_DEPTH_TEST);

        float vertices[] = {
            -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,  0.0f, -1.0f,
            0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,  0.0f, -1.0f,
            0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f,  0.0f, -1.0f,
            0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f,  0.0f, -1.0f,
            -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,  0.0f, -1.0f,
            -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,  0.0f, -1.0f,

            -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 0.0f,  0.0f, 1.0f,
            0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f,  0.0f, 1.0f,
            0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 0.0f,  0.0f, 1.0f,
            0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 0.0f,  0.0f, 1.0f,
            -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f,  0.0f, 1.0f,
            -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 0.0f,  0.0f, 1.0f,

            -0.5f, 0.5f, 0.5f, 1.0f, 0.0f, -1.0f,  0.0f,  0.0f,
            -0.5f, 0.5f, -0.5f, 1.0f, 1.0f, -1.0f,  0.0f,  0.0f,
            -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, -1.0f,  0.0f,  0.0f,
            -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, -1.0f,  0.0f,  0.0f,
            -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, -1.0f,  0.0f,  0.0f,
            -0.5f, 0.5f, 0.5f, 1.0f, 0.0f, -1.0f,  0.0f,  0.0f,

            0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 1.0f,  0.0f,  0.0f,
            0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 1.0f,  0.0f,  0.0f,
            0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 1.0f,  0.0f,  0.0f,
            0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 1.0f,  0.0f,  0.0f,
            0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f,  0.0f,  0.0f,
            0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 1.0f,  0.0f,  0.0f,

            -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, -1.0f,  0.0f,
            0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 0.0f, -1.0f,  0.0f,
            0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, -1.0f,  0.0f,
            0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, -1.0f,  0.0f,
            -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 0.0f, -1.0f,  0.0f,
            -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, -1.0f,  0.0f,

            -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,  1.0f,  0.0f,
            0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f,  1.0f,  0.0f,
            0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f,  1.0f,  0.0f,
            0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f,  1.0f,  0.0f,
            -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 0.0f,  1.0f,  0.0f,
            -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,  1.0f,  0.0f
        };

        cubePositions[0] = glm::vec3(0.0f, 0.0f, 0.0f);
        cubePositions[1] = glm::vec3(2.0f, 5.0f, -15.0f);
        cubePositions[2] = glm::vec3(-1.5f, -2.2f, -2.5f);
        cubePositions[3] = glm::vec3(-3.8f, -2.0f, -12.3f);
        cubePositions[4] = glm::vec3(2.4f, -0.4f, -3.5f);
        cubePositions[5] = glm::vec3(-1.7f, 3.0f, -7.5f);
        cubePositions[6] = glm::vec3(1.3f, -2.0f, -2.5f);
        cubePositions[7] = glm::vec3(1.5f, 2.0f, -2.5f);
        cubePositions[8] = glm::vec3(1.5f, 0.2f, -1.5f);
        cubePositions[9] = glm::vec3(-1.3f, 1.0f, -1.5f);

        unsigned int indices[] = {
            0, 1, 3,
            1, 2, 3
        };


        glGenVertexArrays(1, &vao_);
        glGenBuffers(1, &vbo_);
        glGenBuffers(1, &ebo_);

        // Bind VAO
        glBindVertexArray(vao_);


        // Bind and set VBO
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        // Bind and set EBO
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // TexCoord attribute
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        // normal attribute
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
        glEnableVertexAttribArray(2);

        // Light
        glGenVertexArrays(1, &light_vao_);
        glBindVertexArray(light_vao_);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Unbind VAO (optional)
        glBindVertexArray(0);
    }

    void HelloLight::End()
    {
        //Unload program/pipeline
        glDeleteProgram(program_);
        glDeleteProgram(light_program_);

        glDeleteShader(vertexShader_);
        glDeleteShader(fragmentShader_);
        glDeleteShader(light_vertexShader_);
        glDeleteShader(light_fragmentShader_);

        glDeleteVertexArrays(1, &vao_);
        glDeleteVertexArrays(1, &light_vao_);
        glDeleteBuffers(1, &ebo_);
    }

    void HelloLight::Update(const float dt)
    {
        elapsedTime_ += dt;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // also clear the depth buffer now!

        // Create transformations
        glm::mat4 model = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
        glm::mat4 view = glm::mat4(1.0f);
        glm::mat4 projection = glm::mat4(1.0f);
        model = glm::rotate(model, elapsedTime_, glm::vec3(0.5f, 1.0f, 0.0f));
        view = camera_->view();
        glm::vec3 view_pos = camera_->camera_position_;

        projection = glm::perspective(glm::radians(45.0f), (float)1280 / (float)720, 0.1f, 100.0f);
        // retrieve the matrix uniform locations

        // Pass transformations to shaders TODO: factorize
        unsigned int viewLoc = glGetUniformLocation(program_, "view");
        unsigned int projectionLoc = glGetUniformLocation(program_, "projection");
        unsigned int lightColorLoc = glGetUniformLocation(program_, "lightColor");
        unsigned int lightPosLoc = glGetUniformLocation(program_, "lightPos");
        unsigned int viewPosLoc = glGetUniformLocation(program_, "viewPos");

        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3f(lightColorLoc, 1.0f, 0.0f, 1.0f);
        glUniform3f(lightPosLoc, light_position_.x, light_position_.y, light_position_.z);
        glUniform3f(viewPosLoc, view_pos.x, view_pos.y, view_pos.z);



        //TODO: check why putting this after the cubes make them not appear
        //Draw the lamp object
        glUseProgram(light_program_);

        unsigned int light_viewLoc = glGetUniformLocation(light_program_, "light_view");
        unsigned int light_projectionLoc = glGetUniformLocation(light_program_, "light_projection");

        glUniformMatrix4fv(light_viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(light_projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        glm::mat4 light_model = glm::mat4(1.0f);
        light_model = glm::translate(light_model, light_position_);
        light_model = glm::scale(light_model, glm::vec3(0.2f)); // a smaller cube

        unsigned int light_modelLoc = glGetUniformLocation(light_program_, "light_model");
        glUniformMatrix4fv(light_modelLoc, 1, GL_FALSE, glm::value_ptr(light_model));

        glBindVertexArray(light_vao_);
        glDrawArrays(GL_TRIANGLES, 0, 36);



        //Draw cubes
        glUseProgram(program_);
        glBindTexture(GL_TEXTURE_2D, texture_);
        for (unsigned int i = 0; i < 10; i++)
        {
            // calculate the model matrix for each object and pass it to shader before drawing
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, cubePositions[i]);
            float angle = 20.0f * i;
            model = glm::rotate(model, glm::radians(angle) + elapsedTime_, glm::vec3(1.0f, 0.3f, 0.5f));

            unsigned int modelLoc = glGetUniformLocation(program_, "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

            glBindVertexArray(vao_);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }


        glBindVertexArray(0);
    }

    void HelloLight::OnEvent(const SDL_Event& event, const float dt)
    {
        // Get keyboard state
        const Uint8* state = SDL_GetKeyboardState(NULL);

        // Camera controls
        if (state[SDL_SCANCODE_W])
        {
            camera_->Move(FORWARD, dt);
        }
        if (state[SDL_SCANCODE_S])
        {
            camera_->Move(BACKWARD, dt);
        }
        if (state[SDL_SCANCODE_A])
        {
            camera_->Move(LEFT, dt);
        }
        if (state[SDL_SCANCODE_D])
        {
            camera_->Move(RIGHT, dt);
        }
        if (state[SDL_SCANCODE_SPACE])
        {
            camera_->Move(UP, dt);
        }
        if (state[SDL_SCANCODE_LCTRL])
        {
            camera_->Move(DOWN, dt);
        }

        int mouseX, mouseY;
        SDL_GetRelativeMouseState(&mouseX, &mouseY);
        camera_->Update(mouseX, mouseY);
    }

    void HelloLight::DrawImGui()
    {
        ImGui::Begin("My Window"); // Start a new window
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::End(); // End the window
    }
}

int main(int argc, char** argv)
{
    gpr5300::HelloLight scene;
    gpr5300::Engine engine(&scene);
    engine.Run();

    return EXIT_SUCCESS;
}

