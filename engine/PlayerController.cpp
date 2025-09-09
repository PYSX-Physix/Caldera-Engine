#include "PlayerController.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include "../renderer/Camera.h"

PlayerController::PlayerController(InputSystem* in, Camera* cam, Character* ch) { Attach(in, cam, ch); }

void PlayerController::Attach(InputSystem* in, Camera* cam, Character* ch) {
    input = in; camera = cam; character = ch;
}

void PlayerController::Update(float dt) {
    if (!input || !camera || !character) return;
    input->Update();
    // mouse
    auto md = input->GetMouseDelta();
    float mx = (float)md.first * MouseSensitivity;
    float my = (float)md.second * MouseSensitivity;
    if (input->IsMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT)) {
        camera->ProcessMouseMovement(mx, -my);
    }

    // movement relative to camera
    glm::vec3 forward = glm::normalize(camera->Front);
    glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
    glm::vec3 right = glm::normalize(glm::cross(forward, worldUp));
    glm::vec3 move(0.0f);
    if (input->IsKeyDown(GLFW_KEY_W)) move += forward;
    if (input->IsKeyDown(GLFW_KEY_S)) move -= forward;
    if (input->IsKeyDown(GLFW_KEY_A)) move -= right;
    if (input->IsKeyDown(GLFW_KEY_D)) move += right;
    if (input->IsKeyDown(GLFW_KEY_SPACE)) move += worldUp;
    if (input->IsKeyDown(GLFW_KEY_LEFT_CONTROL)) move -= worldUp;
    if (glm::length(move) > 0.0001f) move = glm::normalize(move) * MoveSpeed;
    character->Velocity = move;
    character->Update(dt);
    camera->Position = character->Position + glm::vec3(0.0f, character->Height * 0.8f, 0.0f);
    input->ResetMouseDelta();
}
