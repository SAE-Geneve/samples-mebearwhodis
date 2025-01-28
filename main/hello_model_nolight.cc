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
#include "model.h"
#include "scene.h"
#include "shader.h"
#include "texture_loader.h"

namespace gpr5300
{
    class HelloModelClean final : public Scene
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

        Model model_;

        float elapsedTime_ = 0.0f;

        float model_scale_ = 1;

        FreeCamera* camera_ = nullptr;
    };

    void HelloModelClean::Begin()
    {
        camera_ = new FreeCamera();
        // stbi_set_flip_vertically_on_load(true);
        glEnable(GL_DEPTH_TEST);

        shader_ = Shader("data/shaders/model/model.vert", "data/shaders/model/model.frag");
        model_ = Model("data/pickle_gltf/Pickle_uishdjrva_Mid.gltf");
    }

    void HelloModelClean::End()
    {
        //Unload program/pipeline
        shader_.Delete();
    }

    void HelloModelClean::Update(const float dt)
    {
        UpdateCamera(dt);
        elapsedTime_ += dt;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // also clear the depth buffer

        shader_.Use();

        // Create transformations
        auto projection = glm::perspective(glm::radians(45.0f), (float)1280 / (float)720, 0.1f, 10000.0f);
        auto view = camera_->view();

        shader_.SetMat4("projection", projection);
        shader_.SetMat4("view", view);

        const glm::vec3 view_pos = camera_->camera_position_;
        shader_.SetVec3("viewPos", view_pos);

        //Draw model
        auto model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::scale(model, model_scale_ * glm::vec3(1.0f, 1.0f, 1.0f));

        shader_.SetMat4("model", model);
        model_.Draw(shader_.id_);

        glBindVertexArray(0);
    }

    void HelloModelClean::OnEvent(const SDL_Event& event)
    {
        switch (event.type)
        {
        case SDL_KEYDOWN:
            if (event.key.keysym.sym == SDLK_LSHIFT)
            {
                camera_->ToggleSprint();
            }
            break;
        default:
            break;
        }
        //TODO: Add zoom
    }

    void HelloModelClean::UpdateCamera(const float dt)
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

    void HelloModelClean::DrawImGui()
    {
        ImGui::Begin("My Window"); // Start a new window
        //ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::SliderFloat("Model Size", &model_scale_, 0.01f, 1.0f, "%.1f");
        static ImVec4 LightColour = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // Default color
        ImGui::End(); // End the window
    }
}

int main(int argc, char* argv[])
{
    gpr5300::HelloModelClean scene;
    gpr5300::Engine engine(&scene);
    engine.Run();

    return EXIT_SUCCESS;
}
