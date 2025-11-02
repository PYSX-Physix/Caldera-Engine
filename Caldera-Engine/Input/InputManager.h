#pragma once

#include <Windows.h>

class InputManager {
public:
    void ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam);
    bool IsKeyPressed(int key);
    POINT GetMousePosition();
};