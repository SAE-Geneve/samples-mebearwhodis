#ifndef FREE_CAMERA_H
#define FREE_CAMERA_H
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_geometric.hpp>

struct FreeCamera {

    glm::vec3 camera_position_ = glm::vec3(0.0f, 0.0f, -3.0f);
    glm::vec3 camera_target_ = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 camera_direction_ = glm::normalize(camera_position_ - camera_target_);

    glm::vec3 world_up_ = glm::vec3(0.0f, 1.0f, 0.0f);

    glm::vec3 camera_right_ = glm::normalize(glm::cross(world_up_, camera_direction_));
    glm::vec3 camera_up_ = glm::normalize(glm::cross(camera_direction_, camera_right_));
    glm::vec3 camera_front_ = glm::vec3(0.0f, 0.0f, 1.0f);

    glm::mat4 view_ = glm::lookAt(camera_position_, camera_position_ + camera_front_, camera_up_);

};
#endif //FREE_CAMERA_H
