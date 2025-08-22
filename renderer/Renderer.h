#pragma once
#include "Shader.h"
#include "Camera.h"
#include "Model.h"
#include <glm/glm.hpp>
#include <vector>

struct Light {
    glm::vec3 Position;
    glm::vec3 Color;
    float     Intensity;
};

class Renderer {
public:
    Renderer();
    ~Renderer();
    bool Init();
    void Resize(int w, int h);
    void Render(const Camera& cam, const Model& model, const std::vector<Light>& lights, const glm::mat4& modelMat);
private:
    Shader* shader = nullptr;
    int width = 1280, height = 720;
};
