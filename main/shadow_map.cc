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
    class ShadowMap final : public Scene
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

        GLuint plane_vao_ = 0;
        GLuint plane_vbo_ = 0;

        GLuint depth_map_fbo_ = 0;
        GLuint depth_map_texture_ = 0;

        unsigned int ground_texture_ = 0;
        unsigned int box_texture_ = 0;

        float elapsedTime_ = 0.0f;

        const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
        glm::vec3 light_position_ = glm::vec3(-2.0f, 4.0f, -1.0f);

        FreeCamera* camera_ = nullptr;
    };

    void ShadowMap::Begin()
    {
        // configure global opengl state
        // -----------------------------
        glEnable(GL_DEPTH_TEST);
        // glDepthFunc(GL_LESS);
        // glEnable(GL_CULL_FACE);
        // glCullFace(GL_FRONT);

        camera_ = new FreeCamera();

        //Build shaders
        shader_ = Shader("data/shaders/shadow_map/shadow_map.vert", "data/shaders/shadow_map/shadow_map.frag");
        shader_depth_ = Shader("data/shaders/shadow_map/shadow_depth.vert",
                               "data/shaders/shadow_map/shadow_depth.frag");
        shader_quad_ = Shader("data/shaders/shadow_map/debug_quad.vert", "data/shaders/shadow_map/debug_quad.frag");

        // set up vertex data (and buffer(s)) and configure vertex attributes
        // ------------------------------------------------------------------
        float planeVertices[] = {
            // positions            // normals         // texcoords
            25.0f, -0.5f, 25.0f, 0.0f, 1.0f, 0.0f, 25.0f, 0.0f,
            -25.0f, -0.5f, 25.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
            -25.0f, -0.5f, -25.0f, 0.0f, 1.0f, 0.0f, 0.0f, 25.0f,

            25.0f, -0.5f, 25.0f, 0.0f, 1.0f, 0.0f, 25.0f, 0.0f,
            -25.0f, -0.5f, -25.0f, 0.0f, 1.0f, 0.0f, 0.0f, 25.0f,
            25.0f, -0.5f, -25.0f, 0.0f, 1.0f, 0.0f, 25.0f, 25.0f
        };
        // plane VAO
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


        //load textures
        ground_texture_ = TextureFromFile("wood.png", "data/textures");
        box_texture_ = TextureFromFile("container2.png", "data/textures");


        // configure depth map FBO
        // -----------------------
        glGenFramebuffers(1, &depth_map_fbo_);
        // create depth texture
        glGenTextures(1, &depth_map_texture_);
        glBindTexture(GL_TEXTURE_2D, depth_map_texture_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT,
                     NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
        // attach depth texture as FBO's depth buffer
        glBindFramebuffer(GL_FRAMEBUFFER, depth_map_fbo_);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_map_texture_, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);


        // shader configuration
        // --------------------
        shader_.Use();
        shader_.SetInt("diffuseTexture", 0);
        shader_.SetInt("shadowMap", 1);
        shader_quad_.Use();
        shader_quad_.SetInt("depthMap", 0);
    }

    void ShadowMap::End()
    {
        shader_.Delete();
        shader_quad_.Delete();
        shader_depth_.Delete();
    }

    void ShadowMap::Update(const float dt)
    {
        UpdateCamera(dt);
        elapsedTime_ += dt;

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 1. render depth of scene to texture (from light's perspective)
        // --------------------------------------------------------------
        glm::mat4 lightProjection, lightView;
        glm::mat4 lightSpaceMatrix;
        float near_plane = 1.0f, far_plane = 7.5f;
        lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
        lightView = glm::lookAt(light_position_, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
        lightSpaceMatrix = lightProjection * lightView;
        // render scene from light's point of view
        shader_depth_.Use();
        shader_depth_.SetMat4("lightSpaceMatrix", lightSpaceMatrix);

        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depth_map_fbo_);
            glClear(GL_DEPTH_BUFFER_BIT);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, ground_texture_);
            renderScene(shader_depth_, plane_vao_);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // reset viewport
        glViewport(0, 0, 1280, 720);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 2. render scene as normal using the generated depth/shadow map
        // --------------------------------------------------------------
        shader_.Use();
        auto projection = glm::perspective(glm::radians(45.0f), (float)1280 / (float)720, 0.1f, 1000.0f);
        auto view = camera_->view();
        shader_.SetMat4("projection", projection);
        shader_.SetMat4("view", view);
        // set light uniforms
        shader_.SetVec3("viewPos", camera_->camera_position_);
        shader_.SetVec3("lightPos", light_position_);
        shader_.SetMat4("lightSpaceMatrix", lightSpaceMatrix);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ground_texture_);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depth_map_texture_);
        renderScene(shader_, plane_vao_);

        // render Depth map to quad for visual debugging
        // ---------------------------------------------
        shader_quad_.Use();
        shader_quad_.SetFloat("near_plane", near_plane);
        shader_quad_.SetFloat("far_plane", far_plane);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, depth_map_texture_);
        //renderQuad();

        glBindVertexArray(0);
    }

    void ShadowMap::OnEvent(const SDL_Event& event)
    {
        //TODO: Add zoom
    }

    void ShadowMap::UpdateCamera(const float dt)
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


    void ShadowMap::DrawImGui()
    {
        ImGui::Begin("My Window"); // Start a new window

        //ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

        // ImGui::Checkbox("Enable Bloom", &bloom_state_);

        // ImGui::SliderFloat("Exposure", &exposure_, 0.1f, 5.0f, "%.1f");

        // static ImVec4 LightColour = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // Default color
        // ImGui::ColorPicker3("Light Colour", reinterpret_cast<float*>(&light_colors_[0]));
        ImGui::End(); // End the window
    }
}


int main(int argc, char* argv[])
{
    gpr5300::ShadowMap scene;
    gpr5300::Engine engine(&scene);
    engine.Run();

    return EXIT_SUCCESS;
}
