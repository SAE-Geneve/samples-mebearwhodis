#include <fstream>
#include <imgui.h>
#include <iostream>
#include <map>
#include <sstream>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "engine.h"
#include "file_utility.h"
#include "free_camera.h"
#include "model.h"
#include "scene.h"
#include "shader.h"
#include "texture_loader.h"

namespace gpr5300
{
    class Cubemap final : public Scene
    {
    public:
        void Begin() override;
        void End() override;
        void Update(float dt) override;
        void OnEvent(const SDL_Event& event) override;
        void DrawImGui() override;
        void UpdateCamera(const float dt) override;

    private:
        Shader shader_ = {};

        Shader skybox_shader_ = {};

        GLuint cube_vao_ = 0;
        GLuint cube_vbo_ = 0;

        GLuint plane_vao_ = 0;
        GLuint plane_vbo_ = 0;

        GLuint skybox_vao_ = 0;
        GLuint skybox_vbo_ = 0;

        unsigned int cubeTexture = -1;
        unsigned int floorTexture = -1;
        unsigned int skybox_texture_ = -1;

        float elapsedTime_ = 0.0f;

        float cube_vertices_[216] = {};
        float plane_vertices_[30] = {};
        float skybox_vertices_[108] = {};

        FreeCamera* camera_ = nullptr;
    };

    void Cubemap::Begin()
    {
        camera_ = new FreeCamera();

        //Main program
        shader_ = Shader("data/shaders/cubemaps/reflection.vert", "data/shaders/cubemaps/reflection.frag");
        skybox_shader_ = Shader("data/shaders/cubemaps/cubemaps.vert", "data/shaders/cubemaps/cubemaps.frag");


        // Configure global opengl state
        // -----------------------------
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        // glEnable(GL_CULL_FACE);
        // glCullFace(GL_FRONT);

        // set up vertex data (and buffer(s)) and configure vertex attributes
        // ------------------------------------------------------------------
        /*
            Remember: to specify vertices in a counter-clockwise winding order you need to visualize the triangle
            as if you're in front of the triangle and from that point of view, is where you set their order.

            To define the order of a triangle on the right side of the cube for example, you'd imagine yourself looking
            straight at the right side of the cube, and then visualize the triangle and make sure their order is specified
            in a counter-clockwise order. This takes some practice, but try visualizing this yourself and see that this
            is correct.
        */

        float cubeVertices[] = {
            // positions          // normals
            -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
            0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
            -0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
            0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
            -0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
            0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f,


            -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
            0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
            0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
            0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
            -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
            -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f,

            -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f,
            -0.5f, 0.5f, -0.5f, -1.0f, 0.0f, 0.0f,
            -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f,
            -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f,
            -0.5f, -0.5f, 0.5f, -1.0f, 0.0f, 0.0f,
            -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f,

            0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
            0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
            0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
            0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
            0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
            0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f,


            -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f,
            -0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f,
            0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f,
            0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f,
            0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f,
            -0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f,


            -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
            0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
            -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
            0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
            -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
            0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f

        };
        std::ranges::copy(cubeVertices, cube_vertices_);

        float planeVertices[] = {
            // positions          // texture Coords (note we set these higher than 1 (together with GL_REPEAT as texture wrapping mode). this will cause the floor texture to repeat)

            -5.0f, -0.5f, -5.0f, 0.0f, 2.0f,
            5.0f, -0.5f, 5.0f, 2.0f, 0.0f,
            5.0f, -0.5f, -5.0f, 2.0f, 2.0f,
            5.0f, -0.5f, 5.0f, 2.0f, 0.0f,
            -5.0f, -0.5f, -5.0f, 0.0f, 2.0f,
            -5.0f, -0.5f, 5.0f, 0.0f, 0.0f
        };

        std::ranges::copy(planeVertices, plane_vertices_);

        float skyboxVertices[] = {
            // positions
            -1.0f, 1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, 1.0f, -1.0f,
            -1.0f, 1.0f, -1.0f,

            -1.0f, -1.0f, 1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f, 1.0f, -1.0f,
            -1.0f, 1.0f, -1.0f,
            -1.0f, 1.0f, 1.0f,
            -1.0f, -1.0f, 1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f, 1.0f,
            -1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, -1.0f, 1.0f,
            -1.0f, -1.0f, 1.0f,

            -1.0f, 1.0f, -1.0f,
            1.0f, 1.0f, -1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            -1.0f, 1.0f, 1.0f,
            -1.0f, 1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f, 1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f, 1.0f,
            1.0f, -1.0f, 1.0f
        };

        std::ranges::copy(skyboxVertices, skybox_vertices_);

        // cube VAO
        glGenVertexArrays(1, &cube_vao_);
        glBindVertexArray(cube_vao_);

        glGenBuffers(1, &cube_vbo_);
        glBindBuffer(GL_ARRAY_BUFFER, cube_vbo_);
        glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices_), &cube_vertices_, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

        // plane VAO
        glGenVertexArrays(1, &plane_vao_);
        glBindVertexArray(plane_vao_);

        glGenBuffers(1, &plane_vbo_);
        glBindBuffer(GL_ARRAY_BUFFER, plane_vbo_);
        glBufferData(GL_ARRAY_BUFFER, sizeof(plane_vertices_), &plane_vertices_, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

        //skybox VAO
        glGenVertexArrays(1, &skybox_vao_);
        glBindVertexArray(skybox_vao_);

        glGenBuffers(1, &skybox_vbo_);
        glBindBuffer(GL_ARRAY_BUFFER, skybox_vbo_);
        glBufferData(GL_ARRAY_BUFFER, sizeof(skybox_vertices_), &skybox_vertices_, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);


        std::vector<std::string> faces
        {
            "data/textures/skybox/right.jpg",
            "data/textures/skybox/left.jpg",
            "data/textures/skybox/top.jpg",
            "data/textures/skybox/bottom.jpg",
            "data/textures/skybox/front.jpg",
            "data/textures/skybox/back.jpg"
        };
        // load textures
        // -------------
        cubeTexture = TextureFromFile("container.jpg", "data/textures");
        floorTexture = TextureFromFile("marble.jpg", "data/textures");
        skybox_texture_ = CubemapFromVec(faces);


        // shader configuration
        // --------------------
        shader_.Use();
        shader_.SetInt("skybox", 0);

        skybox_shader_.Use();
        skybox_shader_.SetInt("skybox", 0);

        // draw as wireframe
        //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    void Cubemap::End()
    {
        //Unload program/pipeline
        shader_.Delete();
        skybox_shader_.Delete();

        glDeleteVertexArrays(1, &cube_vao_);
        glDeleteVertexArrays(1, &plane_vao_);
    }

    void Cubemap::Update(const float dt)
    {
        UpdateCamera(dt);
        elapsedTime_ += dt;


        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader_.Use();

        auto model = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
        auto view = camera_->view();
        auto projection = glm::perspective(glm::radians(45.0f), (float)1280 / (float)720, 0.1f, 100.0f);

        shader_.SetMat4("model", model);
        shader_.SetMat4("view", view);
        shader_.SetMat4("projection", projection);
        shader_.SetVec3("cameraPos", camera_->camera_position_);

        //Cubes
        glBindVertexArray(cube_vao_);
        // glActiveTexture(GL_TEXTURE0);
        // glBindTexture(GL_TEXTURE_2D, cubeTexture);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_texture_);
        model = glm::translate(model, glm::vec3(-1.0f, 0.0f, -1.0f));
        shader_.SetMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(2.0f, 0.0f, 0.0f));
        shader_.SetMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        //Floor
        // glBindVertexArray(plane_vao_);
        // glBindTexture(GL_TEXTURE_2D, floorTexture);
        // model = glm::mat4(1.0f);
        // program_.SetMat4("model", model);
        // glDrawArrays(GL_TRIANGLES, 0, 6);

        //Draw skybox
        glDepthFunc(GL_LEQUAL);
        skybox_shader_.Use();
        view = glm::mat4(glm::mat3(camera_->view()));
        skybox_shader_.SetMat4("view", view);
        skybox_shader_.SetMat4("projection", projection);
        //Skybox cube
        glBindVertexArray(skybox_vao_);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_texture_);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);
    }

    void Cubemap::OnEvent(const SDL_Event& event)
    {
        //TODO: Add zoom
    }

    void Cubemap::UpdateCamera(const float dt)
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
        const Uint32 mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);
        if (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT) && !ImGui::GetIO().WantCaptureMouse)
        {
            camera_->Update(mouseX, mouseY);
        }
    }

    void Cubemap::DrawImGui()
    {
        ImGui::Begin("My Window"); // Start a new window
        //ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        static ImVec4 LightColour = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // Default color
        //ImGui::ColorPicker3("Light Colour", reinterpret_cast<float*>(&light_colour_));
        ImGui::End(); // End the window
    }
}

int main(int argc, char* argv[])
{
    gpr5300::Cubemap scene;
    gpr5300::Engine engine(&scene);
    engine.Run();

    return EXIT_SUCCESS;
}
