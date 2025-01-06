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
    class HDR final : public Scene
    {
    public:
        void Begin() override;
        void End() override;
        void Update(float dt) override;
        void OnEvent(const SDL_Event& event) override;
        void DrawImGui() override;
        void UpdateCamera(const float dt) override;

    private:
        Shader lighting_shader_ = {};
        Shader hdr_shader_ = {};

        GLuint hdr_fbo_ = 0;
        GLuint color_buffer_ = 0;
        GLuint rbo_depth_ = 0;

        unsigned int wall_texture_ = 0;

        float elapsedTime_ = 0.0f;

        std::vector<glm::vec3> light_positions_ = {};
        std::vector<glm::vec3> light_colors_ = {};

        FreeCamera* camera_ = nullptr;

        bool hdr_state_ = true;
        float exposure_ = 1.0f;
    };

    void HDR::Begin()
    {
        // configure global opengl state
        // -----------------------------
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        // glEnable(GL_CULL_FACE);
        // glCullFace(GL_FRONT);

        camera_ = new FreeCamera();

        wall_texture_ = TextureFromFile("brickwall.jpg", "data/textures");

        //Main program(s)
        lighting_shader_ = Shader("data/shaders/hdr/light.vert", "data/shaders/hdr/light.frag");
        hdr_shader_ = Shader("data/shaders/hdr/hdr.vert", "data/shaders/hdr/hdr.frag");

        //Configure FBO
        glGenFramebuffers(1, &hdr_fbo_);

        //Floating point color buffer
        glGenTextures(1, &color_buffer_);
        glBindTexture(GL_TEXTURE_2D, color_buffer_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 1280, 720, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        //Depth buffer (render buffer)
        glGenRenderbuffers(1, &rbo_depth_);
        glBindRenderbuffer(GL_RENDERBUFFER, rbo_depth_);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, 1280, 720);

        //Attach buffers
        glBindFramebuffer(GL_FRAMEBUFFER, hdr_fbo_);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_buffer_, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo_depth_);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            std::cout << "Framebuffer not complete!" << std::endl;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // lighting info
        // -------------
        // positions
        light_positions_.push_back(glm::vec3(0.0f, 0.0f, 49.5f)); // back light
        light_positions_.push_back(glm::vec3(-1.4f, -1.9f, 9.0f));
        light_positions_.push_back(glm::vec3(0.0f, -1.8f, 4.0f));
        light_positions_.push_back(glm::vec3(0.8f, -1.7f, 6.0f));
        // colors
        light_colors_.push_back(glm::vec3(200.0f, 200.0f, 200.0f));
        light_colors_.push_back(glm::vec3(0.1f, 0.0f, 0.0f));
        light_colors_.push_back(glm::vec3(0.0f, 0.0f, 0.2f));
        light_colors_.push_back(glm::vec3(0.0f, 0.1f, 0.0f));

        lighting_shader_.Use();
        lighting_shader_.SetInt("diffuseTexture", 0);

        hdr_shader_.Use();
        hdr_shader_.SetInt("hdrBuffer", 0);
    }

    void HDR::End()
    {
        lighting_shader_.Delete();
        hdr_shader_.Delete();
    }

    void HDR::Update(const float dt)
    {
        UpdateCamera(dt);
        elapsedTime_ += dt;

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 1. render scene into floating point framebuffer
        // -----------------------------------------------
        glBindFramebuffer(GL_FRAMEBUFFER, hdr_fbo_);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        auto projection = glm::perspective(glm::radians(45.0f), (float)1280 / (float)720, 0.1f, 1000.0f);
        auto view = camera_->view();

        lighting_shader_.Use();
        lighting_shader_.SetMat4("projection", projection);
        lighting_shader_.SetMat4("view", view);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, wall_texture_);
        // set lighting uniforms
        for (unsigned int i = 0; i < light_positions_.size(); i++)
        {
            lighting_shader_.SetVec3("lights[" + std::to_string(i) + "].Position", light_positions_[i]);
            lighting_shader_.SetVec3("lights[" + std::to_string(i) + "].Color", light_colors_[i]);
        }
        lighting_shader_.SetVec3("viewPos", camera_->camera_position_);
        // render tunnel
        auto model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 25.0));
        model = glm::scale(model, glm::vec3(2.5f, 2.5f, 27.5f));
        lighting_shader_.SetMat4("model", model);
        lighting_shader_.SetInt("inverse_normals", true);
        renderCube();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // 2. now render floating point color buffer to 2D quad and tonemap HDR colors to default framebuffer's (clamped) color range
        // --------------------------------------------------------------------------------------------------------------------------
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        hdr_shader_.Use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, color_buffer_);
        hdr_shader_.SetInt("hdr", hdr_state_);
        hdr_shader_.SetFloat("exposure", exposure_);
        renderQuad();

        glBindVertexArray(0);
    }

    void HDR::OnEvent(const SDL_Event& event)
    {
        //TODO: Add zoom
    }

    void HDR::UpdateCamera(const float dt)
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


    void HDR::DrawImGui()
    {
        ImGui::Begin("My Window"); // Start a new window

        //ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

        ImGui::Checkbox("Enable HDR", &hdr_state_);

        ImGui::SliderFloat("Exposure", &exposure_, 0.1f, 5.0f, "%.1f");

        // static ImVec4 LightColour = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // Default color
        // ImGui::ColorPicker3("Light Colour", reinterpret_cast<float*>(&light_colors_[0]));
        ImGui::End(); // End the window
    }
}


int main(int argc, char* argv[])
{
    gpr5300::HDR scene;
    gpr5300::Engine engine(&scene);
    engine.Run();

    return EXIT_SUCCESS;
}
