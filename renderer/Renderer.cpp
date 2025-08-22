#include "Renderer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

static const char* vertexSrc = R"GLSL(
#version 150
in vec3 aPos;
in vec3 aNormal;
in vec2 aTex;

out vec3 FragPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)GLSL";

static const char* fragmentSrc = R"GLSL(
#version 150
struct Light { vec3 pos; vec3 color; float intensity; };
const int MAX_LIGHTS = 8;
in vec3 FragPos;
in vec3 Normal;
out vec4 FragColor;

uniform vec3 viewPos;
uniform int numLights;
uniform Light lights[MAX_LIGHTS];

void main() {
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 result = vec3(0.0);
    vec3 base = vec3(0.8, 0.8, 0.8);
    for (int i = 0; i < numLights; ++i) {
        vec3 lightDir = normalize(lights[i].pos - FragPos);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 halfway = normalize(lightDir + viewDir);
        float spec = pow(max(dot(norm, halfway), 0.0), 32.0);
        vec3 ambient = 0.05 * lights[i].color * base;
        vec3 diffuse = diff * lights[i].color * base;
        vec3 specular = spec * lights[i].color * 0.5;
        result += (ambient + diffuse + specular) * lights[i].intensity;
    }
    FragColor = vec4(result, 1.0);
}
)GLSL";

Renderer::Renderer() {}
Renderer::~Renderer() { if (shader) delete shader; }

bool Renderer::Init() {
    shader = new Shader(vertexSrc, fragmentSrc);
    return shader->ID != 0;
}

void Renderer::Resize(int w, int h) { width = w; height = h; }

void Renderer::Render(const Camera& cam, const Model& model, const std::vector<Light>& lights, const glm::mat4& modelMat) {
    if (!shader) return;
    shader->Use();
    glm::mat4 view = cam.GetViewMatrix();
    glm::mat4 proj = cam.GetProjection((float)width / (float)height);
    shader->SetMat4("view", view);
    shader->SetMat4("projection", proj);
    int num = (int)std::min<size_t>(lights.size(), 8);
    shader->SetInt("numLights", num);
    for (int i = 0; i < num; ++i) {
        std::string base = "lights[" + std::to_string(i) + "]";
        shader->SetVec3(base + ".pos", lights[i].Position);
        shader->SetVec3(base + ".color", lights[i].Color);
        shader->SetFloat(base + ".intensity", lights[i].Intensity);
    }
    shader->SetVec3("viewPos", cam.Position);

    // draw meshes with per-mesh transforms if available
    size_t meshCount = model.meshes.size();
    for (size_t i = 0; i < meshCount; ++i) {
        glm::mat4 meshModel = modelMat;
        if (i < model.meshTransforms.size()) meshModel = modelMat * model.meshTransforms[i];
        shader->SetMat4("model", meshModel);
        model.meshes[i].Draw();
    }
}
