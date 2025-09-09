#pragma once
#include <GLFW/glfw3.h>
#include <utility>
#include "imgui.h"

#include <array>

class InputSystem {
public:
    InputSystem(GLFWwindow* w=nullptr, ImGuiIO* io=nullptr);
    void AttachWindow(GLFWwindow* w, ImGuiIO* io);
    void Update();
    bool IsKeyDown(int key) const;
    bool IsMouseButtonDown(int btn) const;
    std::pair<double,double> GetMouseDelta();
    void ResetMouseDelta();
private:
    GLFWwindow* window = nullptr;
    std::array<int, GLFW_KEY_LAST + 1> keyState;
    std::array<int, GLFW_MOUSE_BUTTON_LAST + 1> mouseState;
    double lastX = 0.0, lastY = 0.0;
    double dx = 0.0, dy = 0.0;
    bool first = true;
};
