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
#include "global_utility.h"
#include "model.h"
#include "scene.h"
#include "shader.h"
#include "texture_loader.h"

namespace gpr5300
{
    class NormalMap final : public Scene
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
        Shader light_shader_ = {};

        GLuint wall_vao_ = 0;
        GLuint wall_vbo_ = 0;
        GLuint light_vao_ = 0;

        unsigned int wall_texture_ = 0;
        unsigned int wall_normal_ = 0;

        float elapsedTime_ = 0.0f;

        glm::vec3 pointLightPositions[4] = {};
        glm::vec3 light_position_ = glm::vec3(0.5f, 1.0f, 0.3f);
        glm::vec3 light_colour_ = glm::vec3(1.0f, 1.0f, 1.0f);

        FreeCamera* camera_ = nullptr;
    };

    void NormalMap::Begin()
    {
        // configure global opengl state
        // -----------------------------
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        // glEnable(GL_CULL_FACE);
        // glCullFace(GL_FRONT);

        camera_ = new FreeCamera();

        wall_texture_ = TextureFromFile("brickwall.jpg", "data/textures");
        wall_normal_ = TextureFromFile("brickwall_normal.jpg", "data/textures");

        //Main program(s)
        shader_ = Shader("data/shaders/normal_map/normal_map.vert", "data/shaders/normal_map/normal_map.frag");
        light_shader_ = Shader("data/shaders/hello_light/light.vert", "data/shaders/hello_light/light.frag");

        shader_.Use();
        shader_.SetInt("diffuseMap", 0);
        shader_.SetInt("normalMap", 1);
    }

    void NormalMap::End()
    {
        shader_.Delete();
        light_shader_.Delete();
    }

    void NormalMap::Update(const float dt)
    {
        UpdateCamera(dt);
        elapsedTime_ += dt;

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //Configure transformation matrices
        auto projection = glm::perspective(glm::radians(45.0f), (float)1280 / (float)720, 0.1f, 1000.0f);
        auto view = camera_->view();

        shader_.Use();
        shader_.SetMat4("projection", projection);
        shader_.SetMat4("view", view);

        // render wall
        auto model = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
        model = glm::rotate(model, glm::radians(elapsedTime_ * -10.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0))); // rotate the quad to show normal mapping from multiple directions
        shader_.SetMat4("model", model);
        shader_.SetVec3("viewPos", camera_->camera_position_);
        shader_.SetVec3("lightPos", light_position_);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, wall_texture_);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, wall_normal_);
        normal_renderQuad();

        // render light source (simply re-renders a smaller plane at the light's position for debugging/visualization)
        model = glm::mat4(1.0f);
        model = glm::translate(model, light_position_);
        model = glm::scale(model, glm::vec3(0.1f));
        shader_.SetMat4("model", model);
        normal_renderQuad();

        glBindVertexArray(0);
    }

    void NormalMap::OnEvent(const SDL_Event& event)
    {
        //TODO: Add zoom
    }

    void NormalMap::UpdateCamera(const float dt)
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


    void NormalMap::DrawImGui()
    {
        ImGui::Begin("My Window"); // Start a new window
        //ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        static ImVec4 LightColour = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // Default color
        ImGui::ColorPicker3("Light Colour", reinterpret_cast<float*>(&light_colour_));
        ImGui::End(); // End the window
    }
}



int main(int argc, char* argv[])
{
    gpr5300::NormalMap scene;
    gpr5300::Engine engine(&scene);
    engine.Run();

    return EXIT_SUCCESS;
}
