
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
        GLuint program_ = 0;
        GLuint vertexShader_ = 0;
        GLuint fragmentShader_ = 0;

        GLuint skybox_program_ = 0;
        GLuint skybox_vertexShader_ = 0;
        GLuint skybox_fragmentShader_ = 0;

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

        float cube_vertices_[180] = {};
        float plane_vertices_[30] = {};
        float skybox_vertices_[108] = {};

        FreeCamera* camera_ = nullptr;
    };

    void Cubemap::Begin()
    {
        camera_ = new FreeCamera();

        //Main program

        //Load shaders
        auto vertexContent = LoadFile("data/shaders/depth_testing/depth.vert");
        const auto* ptr = vertexContent.data();
        vertexShader_ = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader_, 1, &ptr, nullptr);
        glCompileShader(vertexShader_);
        //Check success status of shader compilation
        GLint success;
        glGetShaderiv(vertexShader_, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            std::cerr << "Error while loading vertex shader\n";
        }

        auto fragmentContent = LoadFile("data/shaders/depth_testing/depth.frag");
        ptr = fragmentContent.data();
        fragmentShader_ = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader_, 1, &ptr, nullptr);
        glCompileShader(fragmentShader_);
        //Check success status of shader compilation

        glGetShaderiv(fragmentShader_, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            std::cerr << "Error while loading fragment shader\n";
        }
        //Load program/pipeline
        program_ = glCreateProgram();
        glAttachShader(program_, vertexShader_);
        glAttachShader(program_, fragmentShader_);
        glLinkProgram(program_);
        //Check if shader program was linked correctly
        glGetProgramiv(program_, GL_LINK_STATUS, &success);
        if (!success)
        {
            std::cerr << "Error while linking shader program\n";
        }


        auto screen_vertexContent = LoadFile("data/shaders/cubemaps/cubemaps.vert");
        const auto* screen_ptr = screen_vertexContent.data();
        skybox_vertexShader_ = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(skybox_vertexShader_, 1, &screen_ptr, nullptr);
        glCompileShader(skybox_vertexShader_);
        //Check success status of shader compilation
        GLint screen_success;
        glGetShaderiv(skybox_vertexShader_, GL_COMPILE_STATUS, &screen_success);
        if (!screen_success)
        {
            std::cerr << "Error while loading vertex shader\n";
        }

        auto screen_fragmentContent = LoadFile("data/shaders/cubemaps/cubemaps.frag");
        screen_ptr = screen_fragmentContent.data();
        skybox_fragmentShader_ = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(skybox_fragmentShader_, 1, &screen_ptr, nullptr);
        glCompileShader(skybox_fragmentShader_);
        //Check success status of shader compilation

        glGetShaderiv(skybox_fragmentShader_, GL_COMPILE_STATUS, &screen_success);
        if (!screen_success)
        {
            std::cerr << "Error while loading fragment shader\n";
        }
        //Load program/pipeline
        skybox_program_ = glCreateProgram();
        glAttachShader(skybox_program_, skybox_vertexShader_);
        glAttachShader(skybox_program_, skybox_fragmentShader_);
        glLinkProgram(skybox_program_);
        //Check if shader program was linked correctly
        glGetProgramiv(skybox_program_, GL_LINK_STATUS, &screen_success);
        if (!screen_success)
        {
            std::cerr << "Error while linking shader program\n";
        }


        // Configure global opengl state
        // -----------------------------
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glEnable(GL_CULL_FACE);
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
            // Back face
            -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, // Bottom-left
            0.5f, 0.5f, -0.5f, 1.0f, 1.0f, // top-right
            0.5f, -0.5f, -0.5f, 1.0f, 0.0f, // bottom-right
            0.5f, 0.5f, -0.5f, 1.0f, 1.0f, // top-right
            -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, // bottom-left
            -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, // top-left
            // Front face
            -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, // bottom-left
            0.5f, -0.5f, 0.5f, 1.0f, 0.0f, // bottom-right
            0.5f, 0.5f, 0.5f, 1.0f, 1.0f, // top-right
            0.5f, 0.5f, 0.5f, 1.0f, 1.0f, // top-right
            -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, // top-left
            -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, // bottom-left
            // Left face
            -0.5f, 0.5f, 0.5f, 1.0f, 0.0f, // top-right
            -0.5f, 0.5f, -0.5f, 1.0f, 1.0f, // top-left
            -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, // bottom-left
            -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, // bottom-left
            -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, // bottom-right
            -0.5f, 0.5f, 0.5f, 1.0f, 0.0f, // top-right
            // Right face
            0.5f, 0.5f, 0.5f, 1.0f, 0.0f, // top-left
            0.5f, -0.5f, -0.5f, 0.0f, 1.0f, // bottom-right
            0.5f, 0.5f, -0.5f, 1.0f, 1.0f, // top-right
            0.5f, -0.5f, -0.5f, 0.0f, 1.0f, // bottom-right
            0.5f, 0.5f, 0.5f, 1.0f, 0.0f, // top-left
            0.5f, -0.5f, 0.5f, 0.0f, 0.0f, // bottom-left
            // Bottom face
            -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, // top-right
            0.5f, -0.5f, -0.5f, 1.0f, 1.0f, // top-left
            0.5f, -0.5f, 0.5f, 1.0f, 0.0f, // bottom-left
            0.5f, -0.5f, 0.5f, 1.0f, 0.0f, // bottom-left
            -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, // bottom-right
            -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, // top-right
            // Top face
            -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, // top-left
            0.5f, 0.5f, 0.5f, 1.0f, 0.0f, // bottom-right
            0.5f, 0.5f, -0.5f, 1.0f, 1.0f, // top-right
            0.5f, 0.5f, 0.5f, 1.0f, 0.0f, // bottom-right
            -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, // top-left
            -0.5f, 0.5f, 0.5f, 0.0f, 0.0f // bottom-left
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
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f,  1.0f
        };

        std::ranges::copy(skyboxVertices, skybox_vertices_);

        // cube VAO
        glGenVertexArrays(1, &cube_vao_);
        glGenBuffers(1, &cube_vbo_);
        glBindVertexArray(cube_vao_);
        glBindBuffer(GL_ARRAY_BUFFER, cube_vbo_);
        glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices_), &cube_vertices_, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

        // plane VAO
        glGenVertexArrays(1, &plane_vao_);
        glGenBuffers(1, &plane_vbo_);
        glBindVertexArray(plane_vao_);
        glBindBuffer(GL_ARRAY_BUFFER, plane_vbo_);
        glBufferData(GL_ARRAY_BUFFER, sizeof(plane_vertices_), &plane_vertices_, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

        //skybox VAO
        glGenVertexArrays(1, &skybox_vao_);
        glGenBuffers(1, &skybox_vbo_);
        glBindVertexArray(skybox_vao_);
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
        glUseProgram(program_);
        glUniform1i(glGetUniformLocation(program_, "texture1"), 0);

        glUseProgram(skybox_program_);
        glUniform1i(glGetUniformLocation(skybox_program_, "skybox"), 0);

        // draw as wireframe
        //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    void Cubemap::End()
    {
        //Unload program/pipeline
        glDeleteProgram(program_);

        glDeleteShader(vertexShader_);
        glDeleteShader(fragmentShader_);

        glDeleteVertexArrays(1, &cube_vao_);
        glDeleteVertexArrays(1, &plane_vao_);
    }

    void Cubemap::Update(const float dt)
    {
        UpdateCamera(dt);
        elapsedTime_ += dt;


        glEnable(GL_DEPTH_TEST);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(program_);

        auto model = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
        auto view = camera_->view();
        auto projection = glm::perspective(glm::radians(45.0f), (float)1280 / (float)720, 0.1f, 100.0f);

        glUniformMatrix4fv(glGetUniformLocation(program_, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(program_, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        const glm::vec3 view_pos = camera_->camera_position_;
        glUniform3f(glGetUniformLocation(program_, "viewPos"), view_pos.x, view_pos.y, view_pos.z);

        //Cubes
        glBindVertexArray(cube_vao_);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, cubeTexture);
        model = glm::translate(model, glm::vec3(-1.0f, 0.0f, -1.0f));
        glUniformMatrix4fv(glGetUniformLocation(program_, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, 36);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(2.0f, 0.0f, 0.0f));
        glUniformMatrix4fv(glGetUniformLocation(program_, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, 36);
        //Floor
        glBindVertexArray(plane_vao_);
        glBindTexture(GL_TEXTURE_2D, floorTexture);
        model = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(program_, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, 6);

        //Draw skybox
        glDepthFunc(GL_LEQUAL);
        glUseProgram(skybox_program_);
        view = glm::mat4(glm::mat3(camera_->view()));
        glUniformMatrix4fv(glGetUniformLocation(skybox_program_, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(skybox_program_, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        //Skybox cube
        glBindVertexArray(skybox_vao_);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_texture_);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glDepthFunc(GL_LESS);


        glBindVertexArray(0);
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

