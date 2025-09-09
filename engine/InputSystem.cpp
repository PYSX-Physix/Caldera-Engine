#include "InputSystem.h"
#include "imgui.h"
#include <iostream>

InputSystem::InputSystem(GLFWwindow* w, ImGuiIO* io) { AttachWindow(w, io); }

void InputSystem::AttachWindow(GLFWwindow* w, ImGuiIO* io) {
    window = w;
    first = true;
}

void InputSystem::Update() {
    if (!window) return;
    // keys
    for (int k = 0; k <= GLFW_KEY_LAST; ++k) {
        int s = glfwGetKey(window, k);
        keyState[k] = s;
    }
    // mouse buttons
    for (int b = 0; b <= GLFW_MOUSE_BUTTON_LAST; ++b) {
        int s = glfwGetMouseButton(window, b);
        mouseState[b] = s;
    }
    // mouse pos
    double mx, my;
    glfwGetCursorPos(window, &mx, &my);
    if (first) {
        lastX = mx; lastY = my; first = false; dx = dy = 0.0; return;
    }
    dx = mx - lastX;
    dy = my - lastY;
    lastX = mx; lastY = my;
}

bool InputSystem::IsKeyDown(int key) const { if (key < 0 || key > GLFW_KEY_LAST) return false; return keyState[key] == GLFW_PRESS; }
bool InputSystem::IsMouseButtonDown(int btn) const { if (btn < 0 || btn > GLFW_MOUSE_BUTTON_LAST) return false; return mouseState[btn] == GLFW_PRESS; }
std::pair<double,double> InputSystem::GetMouseDelta() { return {dx, dy}; }
void InputSystem::ResetMouseDelta() { dx = dy = 0.0; }
