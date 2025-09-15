#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>

Camera::Camera(glm::vec3 pos) : Position(pos), Front(glm::vec3(0,0,-1)), Up(glm::vec3(0,1,0)) {}

glm::mat4 Camera::GetViewMatrix() const {
    return glm::lookAt(Position, Position + Front, Up);
}

glm::mat4 Camera::GetProjection(float aspect) const {
    return glm::perspective(glm::radians(Fov), aspect, 0.1f, 100.0f);
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch) {
    const float sensitivity = 0.2f;
    xoffset *= -1.0 * sensitivity;
    yoffset *= sensitivity;
    Yaw += xoffset;
    Pitch += yoffset;
    if (constrainPitch) {
        if (Pitch > 89.0f) Pitch = 89.0f;
        if (Pitch < -89.0f) Pitch = -89.0f;
    }
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    Front = glm::normalize(front);
}
