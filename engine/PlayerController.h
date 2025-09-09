#pragma once

#include "InputSystem.h"
#include "GameInputSystem.h"
#include "Character.h"
#include "../renderer/Camera.h"

// PlayerController maps input -> Character movement and camera control.
// It accepts both an editor InputSystem and a GameInputSystem; when playing, GameInputSystem is preferred.
class PlayerController {
public:
    PlayerController(InputSystem* editorIn=nullptr, GameInputSystem* gameIn=nullptr, Camera* cam=nullptr, Character* ch=nullptr);
    void Attach(InputSystem* editorIn, GameInputSystem* gameIn, Camera* cam, Character* ch);
    void Update(float dt);

    float MoveSpeed = 3.0f;
    float MouseSensitivity = 0.1f;

private:
    InputSystem* editorInput = nullptr; // used by editor UI / selection
    GameInputSystem* gameInput = nullptr; // used while playing
    Camera* camera = nullptr;
    Character* character = nullptr;
    bool mouseCaptured = false;
};
