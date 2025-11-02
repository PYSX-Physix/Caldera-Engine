#include "InputManager.h"
#include <unordered_map>

static std::unordered_map<int, bool> keyStates;
static POINT mousePosition = { 0, 0 };

void InputManager::ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_KEYDOWN:
        keyStates[static_cast<int>(wParam)] = true;
        break;
    case WM_KEYUP:
        keyStates[static_cast<int>(wParam)] = false;
        break;
    case WM_MOUSEMOVE:
        mousePosition.x = LOWORD(lParam);
        mousePosition.y = HIWORD(lParam);
        break;
    }
}

bool InputManager::IsKeyPressed(int key) {
    auto it = keyStates.find(key);
    return (it != keyStates.end()) ? it->second : false;
}

POINT InputManager::GetMousePosition() {
    return mousePosition;
}