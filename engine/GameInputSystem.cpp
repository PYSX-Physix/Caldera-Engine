#include "GameInputSystem.h"
#include <GLFW/glfw3.h>
#include <utility>

GameInputSystem::GameInputSystem(GLFWwindow* w) { AttachWindow(w); }
void GameInputSystem::AttachWindow(GLFWwindow* w) { window = w; lastX = lastY = 0.0; dx = dy = 0.0; }
void GameInputSystem::Update() {
    if (!window) return;
    double mx, my;
    glfwGetCursorPos(window, &mx, &my);
    if (lastX == 0.0 && lastY == 0.0) { lastX = mx; lastY = my; dx = dy = 0.0; }
    dx = mx - lastX; dy = my - lastY; lastX = mx; lastY = my;
}
bool GameInputSystem::IsKeyDown(int key) const { if (!window) return false; return glfwGetKey(window, key) == GLFW_PRESS; }
bool GameInputSystem::IsMouseButtonDown(int btn) const { if (!window) return false; return glfwGetMouseButton(window, btn) == GLFW_PRESS; }
std::pair<double,double> GameInputSystem::GetMouseDelta() { return {dx, dy}; }
void GameInputSystem::ResetMouseDelta() { dx = dy = 0.0; }
