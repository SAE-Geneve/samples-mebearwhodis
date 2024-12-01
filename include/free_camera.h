#ifndef FREE_CAMERA_H
#define FREE_CAMERA_H
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_geometric.hpp>

enum Camera_Movement { FORWARD, BACKWARD, LEFT, RIGHT, UP, DOWN };

struct FreeCamera {

    glm::vec3 camera_position_ = glm::vec3(0.0f, 0.0f, -3.0f);
    glm::vec3 camera_target_ = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 camera_direction_ = glm::normalize(camera_position_ - camera_target_);

    glm::vec3 world_up_ = glm::vec3(0.0f, 1.0f, 0.0f);

    glm::vec3 camera_right_ = glm::normalize(glm::cross(world_up_, camera_direction_));
    glm::vec3 camera_up_ = glm::normalize(glm::cross(camera_direction_, camera_right_));
    glm::vec3 camera_front_ = glm::vec3(0.0f, 0.0f, 1.0f);

    glm::mat4 view_ = glm::lookAt(camera_position_, camera_position_ + camera_front_, camera_up_);
    glm::vec3 direction_;
    float yaw_ = -90.0f;
    float pitch_ = 0.0f;
    float sensitivity_ = 0.1f;

    float camera_speed_ = 5.0f;

    void Update(const int x_yaw, const int y_pitch)
    {
        yaw_ += static_cast<float>(x_yaw) * sensitivity_;
        pitch_ -= static_cast<float>(y_pitch) * sensitivity_;
        if(pitch_ > 89.0f)
            pitch_ =  89.0f;
        if(pitch_ < -89.0f)
            pitch_ = -89.0f;
        direction_.x = cos(glm::radians(yaw_)) * cos(glm::radians(pitch_));
        direction_.y = sin(glm::radians(pitch_));
        direction_.z = sin(glm::radians(yaw_)) * cos(glm::radians(pitch_));
        camera_front_ = glm::normalize(direction_);

        view_ = glm::lookAt(camera_position_, camera_position_ + camera_front_, camera_up_);
    }

    void Move(const Camera_Movement direction, const float dt)
    {
        float speed = camera_speed_ * dt;
        if (direction == FORWARD)
            {
                camera_position_ += speed * camera_front_;
            }
        if (direction == BACKWARD)
        {
            camera_position_ -= speed * camera_front_;
        }
        if (direction == LEFT)
        {
            camera_position_ -= glm::normalize(glm::cross(camera_front_, camera_up_)) * speed;
        }
        if (direction == RIGHT)
        {
            camera_position_ += glm::normalize(glm::cross(camera_front_, camera_up_)) * speed;
        }
        if (direction == UP)
        {
            camera_position_ += world_up_ * speed;
        }
        if (direction == DOWN)
        {
            camera_position_ -= world_up_ * speed;
        }
    }

    glm::mat4 view() const { return view_;}

};
#endif //FREE_CAMERA_H
