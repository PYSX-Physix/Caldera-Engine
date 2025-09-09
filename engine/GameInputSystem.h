#pragma once
#include <GLFW/glfw3.h>
#include <utility>

// Separate input system intended for the running game window.
class GameInputSystem {
public:
    GameInputSystem(GLFWwindow* w=nullptr);
    void AttachWindow(GLFWwindow* w);
    void Update();
    bool IsKeyDown(int key) const;
    bool IsMouseButtonDown(int btn) const;
    std::pair<double,double> GetMouseDelta();
    void ResetMouseDelta();
private:
    GLFWwindow* window = nullptr;
    double lastX = 0.0, lastY = 0.0;
    double dx = 0.0, dy = 0.0;
};
