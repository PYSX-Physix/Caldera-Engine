#pragma once
#include "InputSystem.h"
#include "../renderer/Camera.h"
#include "Character.h"

class PlayerController {
public:
    PlayerController(InputSystem* in=nullptr, Camera* cam=nullptr, Character* ch=nullptr);
    void Update(float dt);
    void Attach(InputSystem* in, Camera* cam, Character* ch);
    float MoveSpeed = 3.0f;
    float MouseSensitivity = 0.1f;
private:
    InputSystem* input = nullptr;
    Camera* camera = nullptr;
    Character* character = nullptr;
    bool mouseCaptured = false;
};
