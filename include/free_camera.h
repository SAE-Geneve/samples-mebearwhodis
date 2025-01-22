#ifndef FREE_CAMERA_H
#define FREE_CAMERA_H
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_geometric.hpp>

struct Plane
{
    glm::vec3 normal = { 0.f, 1.f, 0.f }; // unit vector
    float     distance = 0.f;        // Distance with origin

    Plane() = default;

    Plane(const glm::vec3& p1, const glm::vec3& norm)
        : normal(glm::normalize(norm)),
        distance(glm::dot(normal, p1))
    {}

    float getSignedDistanceToPlane(const glm::vec3& point) const
    {
        return glm::dot(normal, point) - distance;
    }
};

struct Frustum
{
    Plane topFace;
    Plane bottomFace;

    Plane rightFace;
    Plane leftFace;

    Plane farFace;
    Plane nearFace;
};

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
    float yaw_ = 90.0f;
    float pitch_ = 0.0f;
    float sensitivity_ = 0.1f;

    float camera_speed_ = 10.0f;
    bool sprint_ = false;

    Frustum frustum_ = {};
    float z_near_;
    float z_far_ ;

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

    void ToggleSprint()
    {
        sprint_ = !sprint_;
    }

    void Move(const Camera_Movement direction, const float dt)
    {
        float base_speed = camera_speed_ * dt;
        float speed = sprint_ ? 10.0f * base_speed : base_speed;
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
        view_ = glm::lookAt(camera_position_, camera_position_ + camera_front_, camera_up_);
    }

    glm::mat4 view() const { return view_;}

    Frustum createFrustum(float aspect, float fovY)
    {
        Frustum     frustum;
        const float halfVSide = z_far_ * tanf(fovY * .5f);
        const float halfHSide = halfVSide * aspect;
        const glm::vec3 frontMultFar = z_far_ * camera_front_;

        frustum.nearFace = { camera_position_ + z_near_ * camera_front_, camera_front_ };
        frustum.farFace = { camera_position_ + frontMultFar, -camera_front_ };
        frustum.rightFace = {camera_position_,
                                glm::cross(frontMultFar - camera_right_ * halfHSide, camera_up_) };
        frustum.leftFace = { camera_position_,
                                glm::cross(camera_up_,frontMultFar + camera_right_ * halfHSide) };
        frustum.topFace = { camera_position_,
                                glm::cross(camera_right_, frontMultFar - camera_up_ * halfVSide) };
        frustum.bottomFace = { camera_position_,
                                glm::cross(frontMultFar + camera_up_ * halfVSide, camera_right_) };

        frustum_ = frustum;
    }

};



#endif //FREE_CAMERA_H
