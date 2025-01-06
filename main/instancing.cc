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
    class Instancing final : public Scene
    {
    public:
        void Begin() override;
        void End() override;
        void Update(float dt) override;
        void OnEvent(const SDL_Event& event) override;
        void DrawImGui() override;
        void UpdateCamera(const float dt) override;

    private:
        Shader planet_shader_ = {};
        Shader asteroid_shader_ = {};

        Shader skybox_program_ = {};
        GLuint skybox_vao_ = 0;
        GLuint skybox_vbo_ = 0;

        unsigned int skybox_texture_ = -1;

        float elapsedTime_ = 0.0f;

        float skybox_vertices_[108] = {};
        float quad_vertices_[30] = {};

        glm::mat4 *modelMatrices = nullptr;
        Model planet_;
        Model asteroid_;
        unsigned int asteroid_amount_ = 100000;
        GLuint asteroid_buffer_ = 0;

        FreeCamera* camera_ = nullptr;
    };

    void Instancing::Begin()
    {
        camera_ = new FreeCamera();

        // stbi_set_flip_vertically_on_load(true);
        planet_ = Model("data/planet/planet.obj");
        asteroid_ = Model("data/rock/rock.obj");


        //Main program(s)
        planet_shader_ = Shader("data/shaders/instancing/planet.vert", "data/shaders/instancing/planet.frag");
        asteroid_shader_ = Shader("data/shaders/instancing/instancing.vert", "data/shaders/instancing/instancing.frag");
        skybox_program_ = Shader("data/shaders/cubemaps/cubemaps.vert", "data/shaders/cubemaps/cubemaps.frag");

        // Configure global opengl state
        // -----------------------------
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        // glEnable(GL_CULL_FACE);
        // glCullFace(GL_FRONT);

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

        modelMatrices = new glm::mat4[asteroid_amount_];
        srand(15678); // initialize random seed
        float radius = 150.0f;
        float offset = 25.0f;
        for(unsigned int i = 0; i < asteroid_amount_; i++)
        {
            glm::mat4 model = glm::mat4(1.0f);
            // 1. translation: displace along circle with 'radius' in range [-offset, offset]
            float angle = (float)i / (float)asteroid_amount_ * 360.0f;
            float displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
            float x = sin(angle) * radius + displacement;
            displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
            float y = displacement * 0.4f; // keep height of field smaller compared to width of x and z
            displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
            float z = cos(angle) * radius + displacement;
            model = glm::translate(model, glm::vec3(x, y, z));

            // 2. scale: scale between 0.05 and 0.25f
            float scale = (rand() % 20) / 100.0f + 0.05;
            model = glm::scale(model, glm::vec3(scale));

            // 3. rotation: add random rotation around a (semi)randomly picked rotation axis vector
            float rotAngle = (rand() % 360);
            model = glm::rotate(model, rotAngle, glm::vec3(0.4f, 0.6f, 0.8f));

            // 4. now add to list of matrices
            modelMatrices[i] = model;
        }

        //Asteroid VBO
        glGenBuffers(1, &asteroid_buffer_);
        glBindBuffer(GL_ARRAY_BUFFER, asteroid_buffer_);
        glBufferData(GL_ARRAY_BUFFER, asteroid_amount_ * sizeof(glm::mat4), &modelMatrices[0], GL_STATIC_DRAW);
        for(unsigned int i = 0; i < asteroid_.meshes().size(); i++)
        {
            unsigned int VAO = asteroid_.meshes()[i].VAO();
            glBindVertexArray(VAO);
            // vertex attributes
            std::size_t vec4Size = sizeof(glm::vec4);
            glEnableVertexAttribArray(3);
            glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)0);
            glEnableVertexAttribArray(4);
            glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(1 * vec4Size));
            glEnableVertexAttribArray(5);
            glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(2 * vec4Size));
            glEnableVertexAttribArray(6);
            glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(3 * vec4Size));

            glVertexAttribDivisor(3, 1);
            glVertexAttribDivisor(4, 1);
            glVertexAttribDivisor(5, 1);
            glVertexAttribDivisor(6, 1);

            glBindVertexArray(0);
        }

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
        // cubeTexture = TextureFromFile("container.jpg", "data/textures");
        skybox_texture_ = CubemapFromVec(faces);


        // shader configuration
        // --------------------
        planet_shader_.Use();

        skybox_program_.Use();
        skybox_program_.SetInt("skybox", 0);

        // TODO: add draw as wireframe in imgui
        //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    void Instancing::End()
    {
        //Unload program/pipeline
        planet_shader_.Delete();
        asteroid_shader_.Delete();
        skybox_program_.Delete();
    }

    void Instancing::Update(const float dt)
    {
        UpdateCamera(dt);
        elapsedTime_ += dt;


        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //Configure transformation matrices
        auto projection = glm::perspective(glm::radians(45.0f), (float)1280 / (float)720, 0.1f, 1000.0f);
        auto view = camera_->view();

        asteroid_shader_.Use();
        asteroid_shader_.SetMat4("projection", projection);
        asteroid_shader_.SetMat4("view", view);
        planet_shader_.Use();
        planet_shader_.SetMat4("projection", projection);
        planet_shader_.SetMat4("view", view);

        // draw planet
        auto model = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
        model = glm::translate(model, glm::vec3(0.0f, -3.0f, 0.0f));
        model = glm::scale(model, glm::vec3(4.0f, 4.0f, 4.0f));
        planet_shader_.SetMat4("model", model);
        planet_.Draw(planet_shader_.id_);

        // draw meteorites
        asteroid_shader_.Use();
        asteroid_shader_.SetInt("texture_diffuse1", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, asteroid_.get_textures_loaded()[0].id);
        for(unsigned int i = 0; i < asteroid_.meshes().size(); i++)
        {
            glBindVertexArray(asteroid_.meshes()[i].VAO());
            glDrawElementsInstanced(GL_TRIANGLES, static_cast<unsigned int>(asteroid_.meshes()[i].indices_.size()), GL_UNSIGNED_INT, 0, asteroid_amount_);
            glBindVertexArray(0);
        }

        //Draw skybox
        glDepthFunc(GL_LEQUAL);
        skybox_program_.Use();
        view = glm::mat4(glm::mat3(camera_->view()));
        skybox_program_.SetMat4("view", view);
        skybox_program_.SetMat4("projection", projection);
        //Skybox cube
        glBindVertexArray(skybox_vao_);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_texture_);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);
    }

    void Instancing::OnEvent(const SDL_Event& event)
    {
        //TODO: Add zoom
    }

    void Instancing::UpdateCamera(const float dt)
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

    void Instancing::DrawImGui()
    {
        ImGui::Begin("My Window"); // Start a new window
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        static ImVec4 LightColour = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // Default color
        //ImGui::ColorPicker3("Light Colour", reinterpret_cast<float*>(&light_colour_));
        ImGui::End(); // End the window
    }
}

int main(int argc, char* argv[])
{
    gpr5300::Instancing scene;
    gpr5300::Engine engine(&scene);
    engine.Run();

    return EXIT_SUCCESS;
}
