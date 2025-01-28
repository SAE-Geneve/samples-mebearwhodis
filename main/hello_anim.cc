#include <fstream>
#include <imgui.h>
#include <iostream>
#include <sstream>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "animation.h"
#include "animator.h"
#include "engine.h"
#include "file_utility.h"
#include "free_camera.h"
#include "model_anim.h"
#include "scene.h"
#include "shader.h"

namespace gpr5300
{
    class HelloAnim final : public Scene
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

        ModelAnim model_;
        Animation animation_ = {};
        Animator animator_ = {};
        float animation_speed_ = 1.0f;

        float elapsedTime_ = 0.0f;

        float model_scale_ = 1;

        FreeCamera* camera_ = nullptr;
    };

    void HelloAnim::Begin()
    {
        camera_ = new FreeCamera();
        // stbi_set_flip_vertically_on_load(true);
        glEnable(GL_DEPTH_TEST);

        shader_ = Shader("data/shaders/hello_anim/hello_anim.vert", "data/shaders/hello_anim/hello_anim.frag");
        model_ = ModelAnim("data/Twist_Dance/Twist_Dance.dae");
        animation_ = Animation("data/Twist_Dance/Twist_Dance.dae", &model_);
        // model_ = ModelAnim("data/jirachi/Model.dae");
        animator_ = Animator(&animation_);
    }

    void HelloAnim::End()
    {
        //Unload program/pipeline
        shader_.Delete();
    }

    void HelloAnim::Update(const float dt)
    {
        UpdateCamera(dt);
        elapsedTime_ += dt;


        animator_.UpdateAnimation(animation_speed_ * dt);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // also clear the depth buffer

        shader_.Use();

        // Create transformations
        auto projection = glm::perspective(glm::radians(45.0f), (float)1280 / (float)720, 0.1f, 10000.0f);
        auto view = camera_->view();

        shader_.SetMat4("projection", projection);
        shader_.SetMat4("view", view);

        const glm::vec3 view_pos = camera_->camera_position_;
        shader_.SetVec3("viewPos", glm::vec3(view_pos.x, view_pos.y, view_pos.z));

        auto transforms = animator_.GetFinalBoneMatrices();
        for (int i = 0; i < transforms.size(); ++i)
        {
            shader_.SetMat4("finalBonesMatrices[" + std::to_string(i) + "]", transforms[i]);
        }

        //Draw model
        auto model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, -0.4f, 0.0f));
        model = glm::scale(model, model_scale_ * glm::vec3(1.0f, 1.0f, 1.0f));

        shader_.SetMat4("model", model);
        model_.Draw(shader_.id_);

        glBindVertexArray(0);
    }

    void HelloAnim::OnEvent(const SDL_Event& event)
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

    void HelloAnim::UpdateCamera(const float dt)
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

    void HelloAnim::DrawImGui()
    {
        ImGui::Begin("My Window"); // Start a new window
        //ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::SliderFloat("Model Size", &model_scale_, 0.00f, 1.0f, "%.05f");
        ImGui::SliderFloat("Animation speed", &animation_speed_, 0.5f, 5.0f, "%.5f");
        static ImVec4 LightColour = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // Default color
        ImGui::End(); // End the window
    }
}

int main(int argc, char* argv[])
{
    gpr5300::HelloAnim scene;
    gpr5300::Engine engine(&scene);
    engine.Run();

    return EXIT_SUCCESS;
}
