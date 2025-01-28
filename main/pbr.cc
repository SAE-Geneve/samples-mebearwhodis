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
    class PBR final : public Scene
    {
    public:
        void Begin() override;
        void End() override;
        void Update(float dt) override;
        void OnEvent(const SDL_Event& event) override;
        void DrawImGui() override;
        void UpdateCamera(const float dt) override;

    private:
        Shader pbr_shader_ = {};
        Shader equirectangular_to_cubemap_shader_ = {};
        Shader irradiance_shader_ = {};
        Shader prefilter_shader_ = {};
        Shader brdf_shader_ = {};
        Shader background_shader_ = {};

        unsigned int irradiance_map_ = 0;
        unsigned int env_cubemap_ = 0;
        unsigned int prefilter_map_ = 0;
        unsigned int brdf_lut_texture_ = 0;

        int nr_rows_ = 7;
        int nr_columns_ = 7;
        float spacing_ = 2.5;

        glm::vec3 light_positions_[4] = {};
        glm::vec3 light_colors_[4] = {};

        float elapsedTime_ = 0.0f;

        FreeCamera* camera_ = nullptr;
    };

    void PBR::Begin()
    {
        camera_ = new FreeCamera();
        // stbi_set_flip_vertically_on_load(true);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);

        pbr_shader_ = Shader("data/shaders/pbr/pbr.vert", "data/shaders/pbr/pbr.frag");
        equirectangular_to_cubemap_shader_ = Shader("data/shaders/pbr/cubemap.vert",
                                                    "data/shaders/pbr/equirectangular_to_cubemap.frag");
        irradiance_shader_ = Shader("data/shaders/pbr/cubemap.vert", "data/shaders/pbr/irradiance.frag");
        prefilter_shader_ = Shader("data/shaders/pbr/cubemap.vert", "data/shaders/pbr/prefilter.frag");
        brdf_shader_ = Shader("data/shaders/pbr/brdf.vert", "data/shaders/pbr/brdf.frag");
        background_shader_ = Shader("data/shaders/pbr/background.vert", "data/shaders/pbr/background.frag");

        pbr_shader_.Use();
        pbr_shader_.SetInt("irradianceMap", 0);
        pbr_shader_.SetInt("prefilterMap", 1);
        pbr_shader_.SetInt("brdfLUT", 2);
        pbr_shader_.SetVec3("albedo", 0.5f, 0.0f, 0.0f);
        pbr_shader_.SetFloat("ao", 1.0f);

        // with Textures
        // pbr_shader_.Use();
        // pbr_shader_.SetInt("irradianceMap", 0);
        // pbr_shader_.SetInt("prefilterMap", 1);
        // pbr_shader_.SetInt("brdfLUT", 2);
        // pbr_shader_.SetInt("albedoMap", 3);
        // pbr_shader_.SetInt("normalMap", 4);
        // pbr_shader_.SetInt("metallicMap", 5);
        // pbr_shader_.SetInt("roughnessMap", 6);
        // pbr_shader_.SetInt("aoMap", 7);

        background_shader_.Use();
        background_shader_.SetInt("environmentMap", 0);

        // you would load PBR material textures here
        // rusted iron
        // unsigned int ironAlbedoMap = loadTexture(FileSystem::getPath("resources/textures/pbr/rusted_iron/albedo.png").c_str());
        // unsigned int ironNormalMap = loadTexture(FileSystem::getPath("resources/textures/pbr/rusted_iron/normal.png").c_str());
        // unsigned int ironMetallicMap = loadTexture(FileSystem::getPath("resources/textures/pbr/rusted_iron/metallic.png").c_str());
        // unsigned int ironRoughnessMap = loadTexture(FileSystem::getPath("resources/textures/pbr/rusted_iron/roughness.png").c_str());
        // unsigned int ironAOMap = loadTexture(FileSystem::getPath("resources/textures/pbr/rusted_iron/ao.png").c_str());

        // lights
        // ------
        glm::vec3 lightPositions[] = {
            glm::vec3(-10.0f, 10.0f, 10.0f),
            glm::vec3(10.0f, 10.0f, 10.0f),
            glm::vec3(-10.0f, -10.0f, 10.0f),
            glm::vec3(10.0f, -10.0f, 10.0f),
        };
        std::ranges::copy(lightPositions, light_positions_);
        glm::vec3 lightColors[] = {
            glm::vec3(300.0f, 300.0f, 300.0f),
            glm::vec3(300.0f, 300.0f, 300.0f),
            glm::vec3(300.0f, 300.0f, 300.0f),
            glm::vec3(300.0f, 300.0f, 300.0f)
        };
        std::ranges::copy(lightColors, light_colors_);


        // pbr: setup framebuffer
        // ----------------------
        unsigned int captureFBO;
        unsigned int captureRBO;
        glGenFramebuffers(1, &captureFBO);
        glGenRenderbuffers(1, &captureRBO);

        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

        // pbr: load the HDR environment map
        // ---------------------------------
        stbi_set_flip_vertically_on_load(true);
        int width, height, nrComponents;
        float* data = stbi_loadf("data/textures/newport_loft.hdr", &width, &height, &nrComponents, 0);
        unsigned int hdrTexture;
        if (data)
        {
            glGenTextures(1, &hdrTexture);
            glBindTexture(GL_TEXTURE_2D, hdrTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);
        }
        else
        {
            std::cout << "Failed to load HDR image." << std::endl;
        }

        // pbr: setup cubemap to render to and attach to framebuffer
        // ---------------------------------------------------------
        glGenTextures(1, &env_cubemap_);
        glBindTexture(GL_TEXTURE_CUBE_MAP, env_cubemap_);
        for (unsigned int i = 0; i < 6; ++i)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 512, 512, 0, GL_RGB, GL_FLOAT, nullptr);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        // glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); //TODO should use this for the prefilter but it makes the cubemap disappear
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // pbr: set up projection and view matrices for capturing data onto the 6 cubemap face directions
        // ----------------------------------------------------------------------------------------------
        glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
        glm::mat4 captureViews[] =
        {
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))
        };

        // pbr: convert HDR equirectangular environment map to cubemap equivalent
        // ----------------------------------------------------------------------
        equirectangular_to_cubemap_shader_.Use();
        equirectangular_to_cubemap_shader_.SetInt("equirectangularMap", 0);
        equirectangular_to_cubemap_shader_.SetMat4("projection", captureProjection);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, hdrTexture);

        glViewport(0, 0, 512, 512); // don't forget to configure the viewport to the capture dimensions.
        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        for (unsigned int i = 0; i < 6; ++i)
        {
            equirectangular_to_cubemap_shader_.SetMat4("view", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                                   env_cubemap_, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            renderCube();
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // pbr: create an irradiance cubemap, and re-scale capture FBO to irradiance scale.
        // --------------------------------------------------------------------------------
        glGenTextures(1, &irradiance_map_);
        glBindTexture(GL_TEXTURE_CUBE_MAP, irradiance_map_);
        for (unsigned int i = 0; i < 6; ++i)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 32, 32, 0, GL_RGB, GL_FLOAT, nullptr);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);

            // pbr: solve diffuse integral by convolution to create an irradiance (cube)map.
    // -----------------------------------------------------------------------------
    irradiance_shader_.Use();
    irradiance_shader_.SetInt("environmentMap", 0);
    irradiance_shader_.SetMat4("projection", captureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, env_cubemap_);

    glViewport(0, 0, 32, 32); // don't forget to configure the viewport to the capture dimensions.
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    for (unsigned int i = 0; i < 6; ++i)
    {
        irradiance_shader_.SetMat4("view", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradiance_map_, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderCube();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // pbr: create a pre-filter cubemap, and re-scale capture FBO to pre-filter scale.
    // --------------------------------------------------------------------------------
    glGenTextures(1, &prefilter_map_);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilter_map_);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // be sure to set minification filter to mip_linear 
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // generate mipmaps for the cubemap so OpenGL automatically allocates the required memory.
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    // pbr: run a quasi monte-carlo simulation on the environment lighting to create a prefilter (cube)map.
    // ----------------------------------------------------------------------------------------------------
    prefilter_shader_.Use();
    prefilter_shader_.SetInt("environmentMap", 0);
    prefilter_shader_.SetMat4("projection", captureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, env_cubemap_);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    unsigned int maxMipLevels = 5;
    for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
    {
        // reisze framebuffer according to mip-level size.
        unsigned int mipWidth  = static_cast<unsigned int>(128 * std::pow(0.5, mip));
        unsigned int mipHeight = static_cast<unsigned int>(128 * std::pow(0.5, mip));
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
        glViewport(0, 0, mipWidth, mipHeight);

        float roughness = (float)mip / (float)(maxMipLevels - 1);
        prefilter_shader_.SetFloat("roughness", roughness);
        for (unsigned int i = 0; i < 6; ++i)
        {
            prefilter_shader_.SetMat4("view", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilter_map_, mip);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            renderCube();
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // pbr: generate a 2D LUT from the BRDF equations used.
    // ----------------------------------------------------
    glGenTextures(1, &brdf_lut_texture_);

    // pre-allocate enough memory for the LUT texture.
    glBindTexture(GL_TEXTURE_2D, brdf_lut_texture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);
    // be sure to set wrapping mode to GL_CLAMP_TO_EDGE
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // then re-configure capture framebuffer object and render screen-space quad with BRDF shader.
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdf_lut_texture_, 0);

    glViewport(0, 0, 512, 512);
    brdf_shader_.Use();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    renderQuad();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // initialize static shader uniforms before rendering
        // --------------------------------------------------
        auto projection = glm::perspective(glm::radians(45.0f), 1280.0f / 720.0f, 0.1f, 100.0f);
        pbr_shader_.Use();
        pbr_shader_.SetMat4("projection", projection);
        background_shader_.Use();
        background_shader_.SetMat4("projection", projection);

        // then before rendering, configure the viewport to the original framebuffer's screen dimensions

        glViewport(0, 0, 1280, 720);
    }

    void PBR::End()
    {
        //Unload program/pipeline
        pbr_shader_.Delete();
        equirectangular_to_cubemap_shader_.Delete();
        irradiance_shader_.Delete();
        background_shader_.Delete();
    }

    void PBR::Update(const float dt)
    {
        UpdateCamera(dt);
        elapsedTime_ += dt;

        // render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // render scene, supplying the convoluted irradiance map to the final shader.
        // ------------------------------------------------------------------------------------------
        pbr_shader_.Use();
        auto view = camera_->view();
        pbr_shader_.SetMat4("view", view);
        const glm::vec3 view_pos = camera_->camera_position_;
        pbr_shader_.SetVec3("camPos", view_pos);

        // bind pre-computed IBL data
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, irradiance_map_);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, prefilter_map_);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, brdf_lut_texture_);

        // // With textures, render each sphere with a different material
        // // rusted iron
        // glActiveTexture(GL_TEXTURE3);
        // glBindTexture(GL_TEXTURE_2D, ironAlbedoMap);
        // glActiveTexture(GL_TEXTURE4);
        // glBindTexture(GL_TEXTURE_2D, ironNormalMap);
        // glActiveTexture(GL_TEXTURE5);
        // glBindTexture(GL_TEXTURE_2D, ironMetallicMap);
        // glActiveTexture(GL_TEXTURE6);
        // glBindTexture(GL_TEXTURE_2D, ironRoughnessMap);
        // glActiveTexture(GL_TEXTURE7);
        // glBindTexture(GL_TEXTURE_2D, ironAOMap);
        //
        // model = glm::mat4(1.0f);
        // model = glm::translate(model, glm::vec3(-5.0, 0.0, 2.0));
        // pbrShader.setMat4("model", model);
        // pbrShader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
        // renderSphere();
        //
        // // gold
        // glActiveTexture(GL_TEXTURE3);
        // glBindTexture(GL_TEXTURE_2D, goldAlbedoMap);
        // glActiveTexture(GL_TEXTURE4);
        // glBindTexture(GL_TEXTURE_2D, goldNormalMap);
        // glActiveTexture(GL_TEXTURE5);
        // glBindTexture(GL_TEXTURE_2D, goldMetallicMap);
        // glActiveTexture(GL_TEXTURE6);
        // glBindTexture(GL_TEXTURE_2D, goldRoughnessMap);
        // glActiveTexture(GL_TEXTURE7);
        // glBindTexture(GL_TEXTURE_2D, goldAOMap);
        //
        // model = glm::mat4(1.0f);
        // model = glm::translate(model, glm::vec3(-3.0, 0.0, 2.0));
        // pbrShader.setMat4("model", model);
        // pbrShader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
        // renderSphere();

        // Without textures, to have all spheres
        // render rows*column number of spheres with varying metallic/roughness values scaled by rows and columns respectively
        glm::mat4 model = glm::mat4(1.0f);
        for (int row = 0; row < nr_rows_; ++row)
        {
            pbr_shader_.SetFloat("metallic", (float)row / (float)nr_rows_);
            for (int col = 0; col < nr_columns_; ++col)
            {
                // we clamp the roughness to 0.025 - 1.0 as perfectly smooth surfaces (roughness of 0.0) tend to look a bit off
                // on direct lighting.
                pbr_shader_.SetFloat("roughness", glm::clamp((float)col / (float)nr_columns_, 0.05f, 1.0f));

                model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(
                                           (float)(col - (nr_columns_ / 2)) * spacing_,
                                           (float)(row - (nr_rows_ / 2)) * spacing_,
                                           -2.0f
                                       ));
                pbr_shader_.SetMat4("model", model);
                pbr_shader_.SetMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
                renderSphere();
            }
        }


        // render light source (simply re-render sphere at light positions)
        // this looks a bit off as we use the same shader, but it'll make their positions obvious and
        // keeps the codeprint small.
        for (unsigned int i = 0; i < sizeof(light_positions_) / sizeof(light_positions_[0]); ++i)
        {
            glm::vec3 newPos = light_positions_[i] + glm::vec3(sin(elapsedTime_ * 5.0) * 5.0, 0.0, 0.0);
            newPos = light_positions_[i];
            pbr_shader_.SetVec3("lightPositions[" + std::to_string(i) + "]", newPos);
            pbr_shader_.SetVec3("lightColors[" + std::to_string(i) + "]", light_colors_[i]);

            model = glm::mat4(1.0f);
            model = glm::translate(model, newPos);
            model = glm::scale(model, glm::vec3(0.5f));
            pbr_shader_.SetMat4("model", model);
            pbr_shader_.SetMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
            renderSphere();
        }

        // render skybox (render as last to prevent overdraw)
        background_shader_.Use();
        background_shader_.SetMat4("view", view);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, env_cubemap_);
        // glBindTexture(GL_TEXTURE_CUBE_MAP, irradiance_map_); // display irradiance map
        // glBindTexture(GL_TEXTURE_CUBE_MAP, prefilter_map_); // display prefilter map
        renderCube();

        // render BRDF map to screen
        //brdf_shader_.Use();
        //renderQuad();
    }

    void PBR::OnEvent(const SDL_Event& event)
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

    void PBR::UpdateCamera(const float dt)
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

    void PBR::DrawImGui()
    {
        ImGui::Begin("My Window"); // Start a new window
        //ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        // ImGui::SliderFloat("Model Size", &model_scale_, 0.01f, 1.0f, "%.1f");
        static ImVec4 LightColour = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // Default color
        ImGui::End(); // End the window
    }
}

int main(int argc, char* argv[])
{
    gpr5300::PBR scene;
    gpr5300::Engine engine(&scene);
    engine.Run();

    return EXIT_SUCCESS;
}
