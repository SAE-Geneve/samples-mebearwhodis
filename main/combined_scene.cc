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
#include "global_utility.h"
#include "model_anim.h"
#include "scene.h"
#include "shader.h"

namespace gpr5300
{
    class CombinedScene final : public Scene
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
        Shader shader_depth_ = {};
        Shader shader_quad_ = {};

        ModelAnim model_;
        Animation animation_ = {};
        Animator animator_ = {};

        GLuint plane_vao_ = 0;
        GLuint plane_vbo_ = 0;
        GLuint depth_map_fbo_ = 0;
        GLuint depth_map_texture_ = 0;

        unsigned int ground_texture_ = 0;
        float animation_speed_ = 1.0f;
        float model_scale_ = 1.0f;

        const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
        glm::vec3 light_position_ = glm::vec3(-2.0f, 4.0f, -1.0f);
        FreeCamera* camera_ = nullptr;
    };

    void CombinedScene::Begin()
    {
        // Camera and OpenGL settings
        camera_ = new FreeCamera();
        glEnable(GL_DEPTH_TEST);

        // Shaders
        // Main scene shader
        shader_ = Shader("data/shaders/combined/combined.vert", "data/shaders/combined/combined.frag");
        // Shadow depth shader
        shader_depth_ = Shader("data/shaders/shadow_map/shadow_depth.vert","data/shaders/shadow_map/shadow_depth.frag");
        // Quad shader (if needed for post-processing)
        shader_quad_ = Shader("data/shaders/shadow_map/debug_quad.vert", "data/shaders/shadow_map/debug_quad.frag");

        // Animated Model
        model_ = ModelAnim("data/Twist_Dance/Twist_Dance.dae");
        animation_ = Animation("data/Twist_Dance/Twist_Dance.dae", &model_);
        animator_ = Animator(&animation_);

        // Plane
        float planeVertices[] = {
            // positions            // normals         // texcoords
            25.0f, -0.5f, 25.0f, 0.0f, 1.0f, 0.0f, 25.0f, 0.0f,
            -25.0f, -0.5f, 25.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
            -25.0f, -0.5f, -25.0f, 0.0f, 1.0f, 0.0f, 0.0f, 25.0f,
            25.0f, -0.5f, 25.0f, 0.0f, 1.0f, 0.0f, 25.0f, 0.0f,
            -25.0f, -0.5f, -25.0f, 0.0f, 1.0f, 0.0f, 0.0f, 25.0f,
            25.0f, -0.5f, -25.0f, 0.0f, 1.0f, 0.0f, 25.0f, 25.0f
        };
        glGenVertexArrays(1, &plane_vao_);
        glGenBuffers(1, &plane_vbo_);
        glBindVertexArray(plane_vao_);
        glBindBuffer(GL_ARRAY_BUFFER, plane_vbo_);
        glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindVertexArray(0);

        // Depth Map FBO
        glGenFramebuffers(1, &depth_map_fbo_);
        glGenTextures(1, &depth_map_texture_);
        glBindTexture(GL_TEXTURE_2D, depth_map_texture_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 1024, 1024, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
        glBindFramebuffer(GL_FRAMEBUFFER, depth_map_fbo_);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_map_texture_, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        shader_.Use();
        shader_.SetInt("diffuseTexture", 0);
        shader_.SetInt("shadowMap", 1);

        // Load textures
        ground_texture_ = TextureFromFile("wood.png", "data/textures");
    }

    void CombinedScene::End()
    {
        shader_.Delete();
        shader_quad_.Delete();
        shader_depth_.Delete();
    }

    void CombinedScene::Update(const float dt)
    {
        // Update camera and animation
        UpdateCamera(dt);
        animator_.UpdateAnimation(animation_speed_ * dt);

        // Shadow Pass
        glViewport(0, 0, 1024, 1024);
        glBindFramebuffer(GL_FRAMEBUFFER, depth_map_fbo_);
        glClear(GL_DEPTH_BUFFER_BIT);
        shader_depth_.Use();

        // Render Plane in Shadow Pass
        glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 7.5f);
        glm::mat4 lightView = glm::lookAt(light_position_, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 lightSpaceMatrix = lightProjection * lightView;
        shader_depth_.SetMat4("lightSpaceMatrix", lightSpaceMatrix);

        auto model = glm::mat4(1.0f);
        shader_depth_.SetMat4("model", model);
        glBindVertexArray(plane_vao_);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Scene Pass
        glViewport(0, 0, 1280, 720);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shader_.Use();

        auto view = camera_->view();
        auto projection = glm::perspective(glm::radians(45.0f), 1280.0f / 720.0f, 0.1f, 100.0f);
        shader_.SetMat4("view", view);
        shader_.SetMat4("projection", projection);

        shader_.SetMat4("lightSpaceMatrix", lightSpaceMatrix);
        shader_.SetVec3("lightPos", light_position_);
        shader_.SetVec3("viewPos", camera_->camera_position_);

        // Render Plane
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ground_texture_);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depth_map_texture_);
        // renderScene(shader_, plane_vao_);

        // Render Animated Model
        auto transforms = animator_.GetFinalBoneMatrices();
        for (int i = 0; i < transforms.size(); ++i)
        {
            shader_.SetMat4("finalBonesMatrices[" + std::to_string(i) + "]", transforms[i]);
        }
        model = glm::translate(model, glm::vec3(0.0f, -0.4f, 0.0f));
        model = glm::scale(model, model_scale_ * glm::vec3(1.0f, 1.0f, 1.0f));

        shader_.SetMat4("model", model);
        model_.Draw(shader_.id_);

    }


    void CombinedScene::OnEvent(const SDL_Event& event)
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

    void CombinedScene::DrawImGui()
    {
        ImGui::SliderFloat("Animation Speed", &animation_speed_, 0.1f, 3.0f);
        ImGui::SliderFloat3("Light Position", &light_position_.x, -10.0f, 10.0f);
        ImGui::SliderFloat("Model Scale", &model_scale_, 0.1f, 2.0f);
    }

    void CombinedScene::UpdateCamera(const float dt)
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
}

int main(int argc, char* argv[])
{
    gpr5300::CombinedScene scene;
    gpr5300::Engine engine(&scene);
    engine.Run();

    return EXIT_SUCCESS;
}
