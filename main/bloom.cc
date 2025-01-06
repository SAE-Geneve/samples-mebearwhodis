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
    class Bloom final : public Scene
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
        Shader shader_light_ = {};
        Shader shader_blur_ = {};
        Shader shader_bloom_final_ = {};

        GLuint hdr_fbo_ = 0;
        GLuint color_buffer_[2] = {};
        GLuint rbo_depth_ = 0;

        //Pingpong for blur
        GLuint pingpong_fbo_[2] = {};
        GLuint pingpong_color_buffer_[2] = {};

        unsigned int ground_texture_ = 0;
        unsigned int box_texture_ = 0;

        float elapsedTime_ = 0.0f;

        std::vector<glm::vec3> light_positions_ = {};
        std::vector<glm::vec3> light_colors_ = {};

        FreeCamera* camera_ = nullptr;

        bool hdr_state_ = true;
        bool bloom_state_ = true;
        float exposure_ = 1.0f;
    };

    void Bloom::Begin()
    {
        // configure global opengl state
        // -----------------------------
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        // glEnable(GL_CULL_FACE);
        // glCullFace(GL_FRONT);

        camera_ = new FreeCamera();

        //Build shaders
        shader_ = Shader("data/shaders/bloom/bloom.vert", "data/shaders/bloom/bloom.frag");
        shader_light_ = Shader("data/shaders/bloom/bloom.vert", "data/shaders/bloom/light.frag");
        shader_blur_ = Shader("data/shaders/bloom/blur.vert", "data/shaders/bloom/blur.frag");
        shader_bloom_final_ = Shader("data/shaders/bloom/bloom_final.vert", "data/shaders/bloom/bloom_final.frag");

        //load textures
        ground_texture_ = TextureFromFile("marble.jpg", "data/textures", true);
        box_texture_ = TextureFromFile("container2.png", "data/textures", true);


        //Configure FBO
        glGenFramebuffers(1, &hdr_fbo_);
        glBindFramebuffer(GL_FRAMEBUFFER, hdr_fbo_);
        //We need 2 floating point color buffers, for normal rendering and brightness thresholds
        glGenTextures(2, color_buffer_);
        for (unsigned int i = 0; i < 2; i++)
        {
            glBindTexture(GL_TEXTURE_2D, color_buffer_[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 1280, 720, 0, GL_RGBA, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            //be sure to clamp to edge!
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, color_buffer_[i], 0);
        }

        //create and attach depth buffer
        glGenRenderbuffers(1, &rbo_depth_);
        glBindRenderbuffer(GL_RENDERBUFFER, rbo_depth_);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, 1280, 720);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo_depth_);
        //select color attachment
        unsigned int attachments[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
        glDrawBuffers(2, attachments);
        // finally check if framebuffer is complete
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "Framebuffer not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        //Pingpong for blur
        glGenFramebuffers(2, pingpong_fbo_);
        glGenTextures(2, pingpong_color_buffer_);
        for (unsigned int i = 0; i < 2; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpong_fbo_[i]);
            glBindTexture(GL_TEXTURE_2D, pingpong_color_buffer_[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 1280, 720, 0, GL_RGBA, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpong_color_buffer_[i], 0);
            // also check if framebuffers are complete (no need for depth buffer)
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                std::cout << "Framebuffer not complete!" << std::endl;
        }

        // lighting info
        // -------------
        // positions
        light_positions_.push_back(glm::vec3(0.0f, 0.5f, 1.5f));
        light_positions_.push_back(glm::vec3(-4.0f, 0.5f, -3.0f));
        light_positions_.push_back(glm::vec3(3.0f, 0.5f, 1.0f));
        light_positions_.push_back(glm::vec3(-.8f, 2.4f, -1.0f));
        // colors
        light_colors_.push_back(glm::vec3(5.0f, 5.0f, 5.0f));
        light_colors_.push_back(glm::vec3(10.0f, 0.0f, 0.0f));
        light_colors_.push_back(glm::vec3(0.0f, 0.0f, 15.0f));
        light_colors_.push_back(glm::vec3(0.0f, 5.0f, 0.0f));


        // shader configuration
        // --------------------
        shader_.Use();
        shader_.SetInt("diffuseTexture", 0);
        shader_blur_.Use();
        shader_blur_.SetInt("image", 0);
        shader_bloom_final_.Use();
        shader_bloom_final_.SetInt("scene", 0);
        shader_bloom_final_.SetInt("bloomBlur", 1);
    }

    void Bloom::End()
    {
        shader_.Delete();
        shader_blur_.Delete();
        shader_light_.Delete();
        shader_bloom_final_.Delete();
    }

    void Bloom::Update(const float dt)
    {
        UpdateCamera(dt);
        elapsedTime_ += dt;

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 1. render scene into floating point framebuffer
        // -----------------------------------------------
        glBindFramebuffer(GL_FRAMEBUFFER, hdr_fbo_);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        auto projection = glm::perspective(glm::radians(45.0f), (float)1280 / (float)720, 0.1f, 1000.0f);
        auto view = camera_->view();
        auto model = glm::mat4(1.0f);
        shader_.Use();
        shader_.SetMat4("projection", projection);
        shader_.SetMat4("view", view);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ground_texture_);
        // set lighting uniforms
        for (unsigned int i = 0; i < light_positions_.size(); i++)
        {
            shader_.SetVec3("lights[" + std::to_string(i) + "].Position", light_positions_[i]);
            shader_.SetVec3("lights[" + std::to_string(i) + "].Color", light_colors_[i]);
        }
        shader_.SetVec3("viewPos", camera_->camera_position_);
        // create one large cube that acts as the floor
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0));
        model = glm::scale(model, glm::vec3(12.5f, 0.5f, 12.5f));
        shader_.SetMat4("model", model);
        renderCube();
        // then create multiple cubes as the scenery
        glBindTexture(GL_TEXTURE_2D, box_texture_);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 1.5f, 0.0));
        model = glm::scale(model, glm::vec3(0.5f));
        shader_.SetMat4("model", model);
        renderCube();

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(2.0f, 0.0f, 1.0));
        model = glm::scale(model, glm::vec3(0.5f));
        shader_.SetMat4("model", model);
        renderCube();

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-1.0f, -1.0f, 2.0));
        model = glm::rotate(model, glm::radians(60.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
        shader_.SetMat4("model", model);
        renderCube();

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 2.7f, 4.0));
        model = glm::rotate(model, glm::radians(23.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
        model = glm::scale(model, glm::vec3(1.25));
        shader_.SetMat4("model", model);
        renderCube();

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-2.0f, 1.0f, -3.0));
        model = glm::rotate(model, glm::radians(124.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
        shader_.SetMat4("model", model);
        renderCube();

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-3.0f, 0.0f, 0.0));
        model = glm::scale(model, glm::vec3(0.5f));
        shader_.SetMat4("model", model);
        renderCube();

        // finally show all the light sources as bright cubes
        shader_light_.Use();
        shader_light_.SetMat4("projection", projection);
        shader_light_.SetMat4("view", view);

        for (unsigned int i = 0; i < light_positions_.size(); i++)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(light_positions_[i]));
            model = glm::scale(model, glm::vec3(0.25f));
            shader_light_.SetMat4("model", model);
            shader_light_.SetVec3("lightColor", light_colors_[i]);
            renderCube();
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // 2. blur bright fragments with two-pass Gaussian Blur
        // --------------------------------------------------
        bool horizontal = true, first_iteration = true;
        unsigned int amount = 10;
        shader_blur_.Use();
        for (unsigned int i = 0; i < amount; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpong_fbo_[horizontal]);
            shader_blur_.SetInt("horizontal", horizontal);
            glBindTexture(GL_TEXTURE_2D, first_iteration ? color_buffer_[1] : pingpong_color_buffer_[!horizontal]);  // bind texture of other framebuffer (or scene if first iteration)
            renderQuad();
            horizontal = !horizontal;
            if (first_iteration)
                first_iteration = false;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // 3. now render floating point color buffer to 2D quad and tonemap HDR colors to default framebuffer's (clamped) color range
        // --------------------------------------------------------------------------------------------------------------------------
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shader_bloom_final_.Use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, color_buffer_[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, pingpong_color_buffer_[!horizontal]);
        shader_bloom_final_.SetInt("bloom", bloom_state_);
        shader_bloom_final_.SetFloat("exposure", exposure_);
        renderQuad();

        glBindVertexArray(0);
    }

    void Bloom::OnEvent(const SDL_Event& event)
    {
        //TODO: Add zoom
    }

    void Bloom::UpdateCamera(const float dt)
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


    void Bloom::DrawImGui()
    {
        ImGui::Begin("My Window"); // Start a new window

        //ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

        ImGui::Checkbox("Enable Bloom", &bloom_state_);

        ImGui::SliderFloat("Exposure", &exposure_, 0.1f, 5.0f, "%.1f");

        // static ImVec4 LightColour = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // Default color
        // ImGui::ColorPicker3("Light Colour", reinterpret_cast<float*>(&light_colors_[0]));
        ImGui::End(); // End the window
    }
}


int main(int argc, char* argv[])
{
    gpr5300::Bloom scene;
    gpr5300::Engine engine(&scene);
    engine.Run();

    return EXIT_SUCCESS;
}
