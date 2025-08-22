#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    float Yaw = -90.0f;
    float Pitch = 0.0f;
    float Fov = 45.0f;

    Camera(glm::vec3 pos = glm::vec3(0,0,3));
    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjection(float aspect) const;
    void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);
};
