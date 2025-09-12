#include "Renderer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include "Texture.h"

// Vertex shader: transforms position, normal, and passes UVs
static const char* vertexSrc = R"GLSL(
#version 150

in vec3 aPos;
in vec3 aNormal;
in vec2 aTex;

out vec3 fragPosition;
out vec3 fragNormal;
out vec2 fragUV;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    fragPosition = vec3(model * vec4(aPos, 1.0));
    fragNormal = mat3(transpose(inverse(model))) * aNormal;
    fragUV = aTex;

    gl_Position = projection * view * vec4(fragPosition, 1.0);
}
)GLSL";

// Fragment shader: applies lighting with diffuse, roughness, and metallic maps
static const char* fragmentSrc = R"GLSL(
#version 150

struct Light {
    vec3 pos;
    vec3 color;
    float intensity;
};

const int MAX_LIGHTS = 8;

in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragUV;

out vec4 FragColor;

uniform vec3 viewPos;
uniform sampler2D diffuseTex;
uniform sampler2D roughnessMap;
uniform sampler2D metallicMap;
uniform int useTexture;
uniform int numLights;
uniform Light lights[MAX_LIGHTS];

void main() {
    vec3 normal = normalize(fragNormal);
    vec3 viewDir = normalize(viewPos - fragPosition);
    vec3 finalColor = vec3(0.0);

    // Sample base color
    vec3 baseColor = vec3(0.8);
    if (useTexture == 1) {
        baseColor = texture(diffuseTex, fragUV).rgb;
    }

    // Sample PBR maps
    float roughness = texture(roughnessMap, fragUV).r;
    float metallic = texture(metallicMap, fragUV).r;

    // Fresnel base reflectance
    vec3 F0 = mix(vec3(0.04), baseColor, metallic);

    for (int i = 0; i < numLights; ++i) {
        vec3 lightDir = normalize(lights[i].pos - fragPosition);
        vec3 halfwayDir = normalize(lightDir + viewDir);

        // Diffuse and specular components
        float diff = max(dot(normal, lightDir), 0.0);
        float specAngle = max(dot(normal, halfwayDir), 0.0);
        float specPower = mix(64.0, 2.0, roughness); // sharper highlights with low roughness
        float specStrength = pow(specAngle, specPower);

        vec3 diffuse = (1.0 - metallic) * baseColor * diff;
        vec3 specular = specStrength * F0;
        vec3 ambient = 0.05 * lights[i].color * baseColor;

        finalColor += (ambient + diffuse + specular) * lights[i].intensity;
    }

    FragColor = vec4(clamp(finalColor, 0.0, 1.0), 1.0);
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
        // bind model texture if provided
        if (model.GetTexture()) {
            model.GetTexture()->Bind(0);
            shader->SetInt("diffuseTex", 0);
            shader->SetInt("useTexture", 1);
        } else {
            shader->SetInt("useTexture", 0);
        }
        if (model.GetRoughnessMap()) {
            model.GetRoughnessMap()->Bind(1);
            shader->SetInt("roughnessMap", 1);
        }
        if (model.GetMetallicMap()) {
            model.GetMetallicMap()->Bind(2);
            shader->SetInt("metallicMap", 2);
        }

        model.meshes[i].Draw();
    }
}
