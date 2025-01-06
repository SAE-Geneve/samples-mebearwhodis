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
#include "texture_loader.h"

namespace gpr5300
{
    class HelloModel final : public Scene
    {
    public:
        void Begin() override;
        void End() override;
        void Update(float dt) override;
        void OnEvent(const SDL_Event& event) override;
        void DrawImGui() override;
        void UpdateCamera(const float dt) override;

    private:
        GLuint vertexShader_ = 0;
        GLuint fragmentShader_ = 0;
        GLuint light_vertexShader_ = 0;
        GLuint light_fragmentShader_ = 0;
        GLuint program_ = 0;
        GLuint light_program_ = 0;
        GLuint vao_ = 0;
        GLuint vbo_ = 0;
        GLuint ebo_ = 0;
        GLuint light_vao_ = 0;
        unsigned int diffuse_map_ = -1;
        unsigned int specular_map_ = -1;

        Model model_;

        float elapsedTime_ = 0.0f;

        glm::vec3 cubePositions[10] = {};
        glm::vec3 pointLightPositions[4] = {};
        glm::vec3 light_position_ = glm::vec3(1.2f, 1.0f, 2.0f);
        glm::vec3 light_colour_ = glm::vec3(1.0f, 1.0f, 1.0f);

        FreeCamera* camera_ = nullptr;
    };

    void HelloModel::Begin()
    {
        camera_ = new FreeCamera();
        stbi_set_flip_vertically_on_load(true);
        // model_ = Model("data/backpack/backpack.obj");
        model_ = Model("data/pickle_uishdjrva_mid/Pickle_uishdjrva_Mid.fbx");
        // model_ = Model("data/dark-sun-gwyndolin/source/Dark_Sun_Gwyndolin/Gwyndolin.fbx");
        // model_ = Model("data/matilda/source/sketchfab_v002.fbx");
        diffuse_map_ = TextureFromFile("container2.png", "data/textures");
        specular_map_ = TextureFromFile("container2_specular.png", "data/textures");
        //Main program
        //Load shaders
        const auto vertexContent = LoadFile("data/shaders/hello_triangle/triangle.vert");
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
        const auto fragmentContent = LoadFile("data/shaders/hello_triangle/triangle.frag");
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

        //Light program
        //Load shaders
        const auto lightVertexContent = LoadFile("data/shaders/hello_light/light.vert");
        ptr = lightVertexContent.data();
        light_vertexShader_ = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(light_vertexShader_, 1, &ptr, nullptr);
        glCompileShader(light_vertexShader_);
        //Check success status of shader compilation
        glGetShaderiv(light_vertexShader_, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            std::cerr << "Error while loading vertex shader\n";
        }
        const auto lightFragmentContent = LoadFile("data/shaders/hello_light/light.frag");
        ptr = lightFragmentContent.data();
        light_fragmentShader_ = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(light_fragmentShader_, 1, &ptr, nullptr);
        glCompileShader(light_fragmentShader_);
        //Check success status of shader compilation

        glGetShaderiv(light_fragmentShader_, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            std::cerr << "Error while loading fragment shader\n";
        }
        //Load program/pipeline
        light_program_ = glCreateProgram();
        glAttachShader(light_program_, light_vertexShader_);
        glAttachShader(light_program_, light_fragmentShader_);
        glLinkProgram(light_program_);
        //Check if shader program was linked correctly

        glGetProgramiv(light_program_, GL_LINK_STATUS, &success);
        if (!success)
        {
            std::cerr << "Error while linking shader program\n";
        }

        // configure global opengl state
        // -----------------------------
        glEnable(GL_DEPTH_TEST);

        float vertices[] = {
            // positions          // normals           // texture coords
            -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
            0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,
            0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
            0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
            -0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,
            -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,

            -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
            0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
            0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
            0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
            -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
            -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,

            -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            -0.5f, 0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
            -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            -0.5f, -0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,

            0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
            0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,

            -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,
            0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f,
            0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
            0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
            -0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f,
            -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,

            -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
            0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
            0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
            0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
            -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
            -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f
        };

        cubePositions[0] = glm::vec3(0.0f, 0.0f, 0.0f);
        cubePositions[1] = glm::vec3(2.0f, 5.0f, -15.0f);
        cubePositions[2] = glm::vec3(-1.5f, -2.2f, -2.5f);
        cubePositions[3] = glm::vec3(-3.8f, -2.0f, -12.3f);
        cubePositions[4] = glm::vec3(2.4f, -0.4f, -3.5f);
        cubePositions[5] = glm::vec3(-1.7f, 3.0f, -7.5f);
        cubePositions[6] = glm::vec3(1.3f, -2.0f, -2.5f);
        cubePositions[7] = glm::vec3(1.5f, 2.0f, -2.5f);
        cubePositions[8] = glm::vec3(1.5f, 0.2f, -1.5f);
        cubePositions[9] = glm::vec3(-1.3f, 1.0f, -1.5f);

        // positions of the point lights
        pointLightPositions[0] = glm::vec3(0.7f, 0.2f, 2.0f),
            pointLightPositions[1] = glm::vec3(2.3f, -3.3f, -4.0f),
            pointLightPositions[2] = glm::vec3(-4.0f, 2.0f, -12.0f),
            pointLightPositions[3] = glm::vec3(0.0f, 0.0f, -3.0f);

        unsigned int indices[] = {
            0, 1, 3,
            1, 2, 3
        };


        glGenVertexArrays(1, &vao_);
        glGenBuffers(1, &vbo_);
        glGenBuffers(1, &ebo_);

        // Bind VAO
        glBindVertexArray(vao_);


        // Bind and set VBO
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        // Bind and set EBO
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // normal attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        // TexCoord attribute
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);


        // Light
        glGenVertexArrays(1, &light_vao_);
        glBindVertexArray(light_vao_);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Unbind VAO (optional)
        glBindVertexArray(0);
    }

    void HelloModel::End()
    {
        //Unload program/pipeline
        glDeleteProgram(program_);
        glDeleteProgram(light_program_);

        glDeleteShader(vertexShader_);
        glDeleteShader(fragmentShader_);
        glDeleteShader(light_vertexShader_);
        glDeleteShader(light_fragmentShader_);

        glDeleteVertexArrays(1, &vao_);
        glDeleteVertexArrays(1, &light_vao_);
        glDeleteBuffers(1, &ebo_);
    }

    void HelloModel::Update(const float dt)
    {
        UpdateCamera(dt);
        elapsedTime_ += dt;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // also clear the depth buffer

        glUseProgram(program_);

        // Create transformations
        auto projection = glm::perspective(glm::radians(45.0f), (float)1280 / (float)720, 0.1f, 100.0f);
        auto view = camera_->view();
        auto model = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first

        // retrieve the matrix uniform locations
        // Pass transformations to shaders TODO: factorize
        glUniformMatrix4fv(glGetUniformLocation(program_, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(program_, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(program_, "model"), 1, GL_FALSE, glm::value_ptr(model));

        const glm::vec3 view_pos = camera_->camera_position_;
        glUniform3f(glGetUniformLocation(program_, "viewPos"), view_pos.x, view_pos.y, view_pos.z);

        //Light
        //Directional light (sun)
        glUniform3f(glGetUniformLocation(program_, "dirLight.direction"), -0.2f, -1.0f, -0.3f);
        glUniform3f(glGetUniformLocation(program_, "dirLight.ambient"), 1.000f, 1.000f, 1.000f);
        glUniform3f(glGetUniformLocation(program_, "dirLight.diffuse"), 1.0f, 1.0f, 1.0f);
        glUniform3f(glGetUniformLocation(program_, "dirLight.specular"), 1.0f, 1.0f, 1.0f);

        //Point Light 1
        glUniform3f(glGetUniformLocation(program_, "pointLights[0].position"), pointLightPositions[0].x,
                    pointLightPositions[0].y, pointLightPositions[0].z);
        glUniform3f(glGetUniformLocation(program_, "pointLights[0].ambient"), 0.05f, 0.05f, 0.05f);
        glUniform3f(glGetUniformLocation(program_, "pointLights[0].diffuse"), light_colour_.r, light_colour_.g,
                    light_colour_.b);
        glUniform3f(glGetUniformLocation(program_, "pointLights[0].specular"), 1.0f, 1.0f, 1.0f);
        glUniform1f(glGetUniformLocation(program_, "pointLights[0].constant"), 1.0f);
        glUniform1f(glGetUniformLocation(program_, "pointLights[0].linear"), 0.09f);
        glUniform1f(glGetUniformLocation(program_, "pointLights[0].quadratic"), 0.032f);

        //Point Light 2
        glUniform3f(glGetUniformLocation(program_, "pointLights[1].position"), pointLightPositions[1].x,
                    pointLightPositions[1].y, pointLightPositions[1].z);
        glUniform3f(glGetUniformLocation(program_, "pointLights[1].ambient"), 0.05f, 0.05f, 0.05f);
        glUniform3f(glGetUniformLocation(program_, "pointLights[1].diffuse"), light_colour_.r, light_colour_.g,
                    light_colour_.b);
        glUniform3f(glGetUniformLocation(program_, "pointLights[1].specular"), 1.0f, 1.0f, 1.0f);
        glUniform1f(glGetUniformLocation(program_, "pointLights[1].constant"), 1.0f);
        glUniform1f(glGetUniformLocation(program_, "pointLights[1].linear"), 0.09f);
        glUniform1f(glGetUniformLocation(program_, "pointLights[1].quadratic"), 0.032f);

        //Point Light 3
        glUniform3f(glGetUniformLocation(program_, "pointLights[2].position"), pointLightPositions[2].x,
                    pointLightPositions[2].y, pointLightPositions[2].z);
        glUniform3f(glGetUniformLocation(program_, "pointLights[2].ambient"), 0.05f, 0.05f, 0.05f);
        glUniform3f(glGetUniformLocation(program_, "pointLights[2].diffuse"), light_colour_.r, light_colour_.g,
                    light_colour_.b);
        glUniform3f(glGetUniformLocation(program_, "pointLights[2].specular"), 1.0f, 1.0f, 1.0f);
        glUniform1f(glGetUniformLocation(program_, "pointLights[2].constant"), 1.0f);
        glUniform1f(glGetUniformLocation(program_, "pointLights[2].linear"), 0.09f);
        glUniform1f(glGetUniformLocation(program_, "pointLights[2].quadratic"), 0.032f);

        //Point Light 4
        glUniform3f(glGetUniformLocation(program_, "pointLights[3].position"), pointLightPositions[3].x,
                    pointLightPositions[3].y, pointLightPositions[3].z);
        glUniform3f(glGetUniformLocation(program_, "pointLights[3].ambient"), 0.05f, 0.05f, 0.05f);
        glUniform3f(glGetUniformLocation(program_, "pointLights[3].diffuse"), light_colour_.r, light_colour_.g,
                    light_colour_.b);
        glUniform3f(glGetUniformLocation(program_, "pointLights[3].specular"), 1.0f, 1.0f, 1.0f);
        glUniform1f(glGetUniformLocation(program_, "pointLights[3].constant"), 1.0f);
        glUniform1f(glGetUniformLocation(program_, "pointLights[3].linear"), 0.09f);
        glUniform1f(glGetUniformLocation(program_, "pointLights[3].quadratic"), 0.032f);

        //Flashlight
        glUniform3f(glGetUniformLocation(program_, "spotLight.position"), camera_->camera_position_.x,
                    camera_->camera_position_.y, camera_->camera_position_.z);
        glUniform3f(glGetUniformLocation(program_, "spotLight.direction"), camera_->camera_front_.x,
                    camera_->camera_front_.y, camera_->camera_front_.z);
        glUniform1f(glGetUniformLocation(program_, "spotLight.cutOff"), glm::cos(glm::radians(12.5f)));
        glUniform1f(glGetUniformLocation(program_, "spotLight.outerCutOff"), glm::cos(glm::radians(17.5f)));
        glUniform3f(glGetUniformLocation(program_, "spotLight.ambient"), 0.0f, 0.0f, 0.0f);
        glUniform3f(glGetUniformLocation(program_, "spotLight.diffuse"), 1.0f, 1.0f, 1.0f);
        glUniform3f(glGetUniformLocation(program_, "spotLight.specular"), 1.0f, 1.0f, 1.0f);
        glUniform1f(glGetUniformLocation(program_, "spotLight.constant"), 1.0f);
        glUniform1f(glGetUniformLocation(program_, "spotLight.linear"), 0.09f);
        glUniform1f(glGetUniformLocation(program_, "spotLight.quadratic"), 0.032f);

        //Material
        glUniform1f(glGetUniformLocation(program_, "material.shininess"), 32.0f);


        glBindVertexArray(vao_);
        //Draw model
        glUniform1i(glGetUniformLocation(program_, "material.diffuse"), 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuse_map_);
        model = glm::translate(model, glm::vec3(2.0f, 0.0f, 0.0f)); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));	// it's a bit too big for our scene, so scale it down
        glUniformMatrix4fv(glGetUniformLocation(program_, "model"), 1, GL_FALSE, glm::value_ptr(model));
        model_.Draw(program_);

        //Bind texture maps
        glUniform1i(glGetUniformLocation(program_, "material.diffuse"), 1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, diffuse_map_);

        glUniform1i(glGetUniformLocation(program_, "material.specular"), 2);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, specular_map_);


        glBindVertexArray(vao_);
        //Draw cubes
        for (unsigned int i = 0; i < 10; i++)
        {
            // calculate the model matrix for each object and pass it to shader before drawing
            auto box_model = glm::mat4(1.0f);
            box_model = glm::translate(box_model, cubePositions[i]);
            float angle = 20.0f * i;
            box_model = glm::rotate(box_model, glm::radians(angle) + elapsedTime_, glm::vec3(1.0f, 0.3f, 0.5f));

            glUniformMatrix4fv(glGetUniformLocation(program_, "model"), 1, GL_FALSE, glm::value_ptr(box_model));

            glDrawArrays(GL_TRIANGLES, 0, 36);
        }


        //Draw the lamp objects
        glUseProgram(light_program_);

        glUniformMatrix4fv(glGetUniformLocation(light_program_, "light_view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(light_program_, "light_projection"), 1, GL_FALSE,
                           glm::value_ptr(projection));
        glUniform3f(glGetUniformLocation(light_program_, "lightColour"), light_colour_.r, light_colour_.g,
                    light_colour_.b);

        glBindVertexArray(light_vao_);
        for (unsigned int i = 0; i < 4; i++)
        {
            glm::mat4 light_model = glm::mat4(1.0f);
            light_model = glm::translate(light_model, pointLightPositions[i]);
            light_model = glm::scale(light_model, glm::vec3(0.2f));

            glUniformMatrix4fv(glGetUniformLocation(light_program_, "light_model"), 1, GL_FALSE,
                               glm::value_ptr(light_model));

            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        glBindVertexArray(0);
    }

    void HelloModel::OnEvent(const SDL_Event& event)
    {
        //TODO: Add zoom
    }

    void HelloModel::UpdateCamera(const float dt)
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

    void HelloModel::DrawImGui()
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
    gpr5300::HelloModel scene;
    gpr5300::Engine engine(&scene);
    engine.Run();

    return EXIT_SUCCESS;
}

