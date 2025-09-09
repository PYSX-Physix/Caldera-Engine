#include "Character.h"

Character::Character() : Position(0.0f), Velocity(0.0f) {}

void Character::Update(float dt) {
    // simple integration for now
    Position += Velocity * dt;
    // damping
    Velocity *= 0.9f;
}
