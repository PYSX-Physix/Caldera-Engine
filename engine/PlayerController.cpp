#include "PlayerController.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

PlayerController::PlayerController(InputSystem* editorIn, GameInputSystem* gameIn, Camera* cam, Character* ch) {
    Attach(editorIn, gameIn, cam, ch);
}

void PlayerController::Attach(InputSystem* editorIn, GameInputSystem* gameIn, Camera* cam, Character* ch) {
    editorInput = editorIn; gameInput = gameIn; camera = cam; character = ch;
}

void PlayerController::Update(float dt) {
    // Prefer game input when attached (playing); otherwise editor input
    if (!camera || !character) return;
    GameInputSystem* in = gameInput;
    if (!in && editorInput) {
        // use editor input as a fallback (it polls GLFW directly)
        // wrap editorInput to mimic GameInputSystem calls by reading mouse delta/state
        editorInput->Update();
    }

    // get mouse delta from appropriate source
    double mdx = 0.0, mdy = 0.0;
    if (gameInput) {
        auto d = gameInput->GetMouseDelta(); mdx = d.first; mdy = d.second;
    } else if (editorInput) { auto d = editorInput->GetMouseDelta(); mdx = d.first; mdy = d.second; }
    float mx = (float)mdx * MouseSensitivity;
    float my = (float)mdy * MouseSensitivity;
    bool rightDown = false;
    if (gameInput) rightDown = gameInput->IsMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT);
    else if (editorInput) rightDown = editorInput->IsMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT);
    if (rightDown) camera->ProcessMouseMovement(mx, -my);

    // movement relative to camera
    glm::vec3 forward = glm::normalize(camera->Front);
    glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
    glm::vec3 right = glm::normalize(glm::cross(forward, worldUp));
    glm::vec3 move(0.0f);
    auto isKey = [&](int k)->bool {
        if (gameInput) return gameInput->IsKeyDown(k);
        if (editorInput) return editorInput->IsKeyDown(k);
        return false;
    };
    if (isKey(GLFW_KEY_W)) move += forward;
    if (isKey(GLFW_KEY_S)) move -= forward;
    if (isKey(GLFW_KEY_A)) move -= right;
    if (isKey(GLFW_KEY_D)) move += right;
    if (isKey(GLFW_KEY_SPACE)) move += worldUp;
    if (isKey(GLFW_KEY_LEFT_CONTROL)) move -= worldUp;
    if (glm::length(move) > 0.0001f) move = glm::normalize(move) * MoveSpeed;
    character->Velocity = move;
    character->Update(dt);
    camera->Position = character->Position + glm::vec3(0.0f, character->Height * 0.8f, 0.0f);
    if (gameInput) gameInput->ResetMouseDelta();
    if (editorInput) editorInput->ResetMouseDelta();
}
