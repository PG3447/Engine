#ifndef COMPONENT_H
#define COMPONENT_H

#include <glm/glm.hpp>


struct Component {
    virtual ~Component() {}
};


struct TransformComponent : Component {
    static constexpr uint64_t ComponentBit = 1ull << 0;

    glm::vec3 position{ 0.0f, 0.0f, 0.0f };
    glm::vec3 rotation{ 0.0f, 0.0f, 0.0f };
    glm::vec3 scale{ 1.0f, 1.0f, 1.0f };

    glm::mat4 modelMatrix{ 1.0f };
    bool isDirty = true;
};

struct RigidbodyComponent : Component {
    static constexpr uint64_t ComponentBit = 1ull << 1;

    float mass = 0;
    float vx = 0, vy = 0;
};


#endif