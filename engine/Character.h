#pragma once
#include <glm/glm.hpp>

class Character {
public:
    Character();
    glm::vec3 Position;
    glm::vec3 Velocity;
    float Height = 1.8f;
    void Update(float dt);
};
