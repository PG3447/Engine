#ifndef COMPONENT_H
#define COMPONENT_H

#include <glm/glm.hpp>
#include <model.h>
#include <shader.h>
#include <camera.h>

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


struct RenderComponent : Component {
    static constexpr uint64_t ComponentBit = 1ull << 1;

    Model* model = nullptr;
    Shader* shader = nullptr;
};


struct RigidbodyComponent : Component {
    static constexpr uint64_t ComponentBit = 1ull << 2;

    float mass = 0;
    float vx = 0, vy = 0;
};

/*
struct CameraComponent : Component {
    static constexpr uint64_t ComponentBit = 1ull << 3;

    float fov = 45.0f;
    float aspectRatio = 16.0f / 9.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;

    glm::mat4 view{ 1.0f };
    glm::mat4 projection{ 1.0f };

    bool isActive = true;
};
*/

struct CameraComponent : Component {
    static constexpr uint64_t ComponentBit = 1ull << 3;

    Camera camera;

    float nearPlane = 0.1f;
    float farPlane = 10000.0f;

    bool isActive = true;
};


#endif