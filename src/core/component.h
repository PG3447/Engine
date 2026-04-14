#ifndef COMPONENT_H
#define COMPONENT_H

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include "camera.h"

class Model;
class Shader;

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

struct SpriteComponent : Component {
    static constexpr uint64_t ComponentBit = 1ull << 4;

    std::vector<std::shared_ptr<unsigned int>> sprites;
    int currentSprite = 0;

    float frameDuration = 0.1f;
    float elapsedTime = 0.0f;
    bool isAnimating = false;
    bool loop = false;

    glm::vec2 screenPosition{ 0.0f, 0.0f };
    glm::vec2 size{ 64.0f, 64.0f };
    float opacity = 1.0f;
    bool isVisible = true;
    int layer = 0;

    bool scrollEnabled = false;
    glm::vec2 scrollSpeed = { 0.0f, 0.0f };
    glm::vec2 scrollOffset = { 0.0f, 0.0f };

    bool textEnabled = false;
    std::string text = "";
    std::string fontPath = "res/textures/fonts/arial.ttf";
    float fontSize = 32.0f;
    glm::vec3 textColor = { 0.0f, 0.0f, 0.0f };
    glm::vec2 textOffset = { 0.0f, 0.0f };

    unsigned int currentTextureID() const {
        if (sprites.empty() || !sprites[currentSprite]) return 0;
        return *sprites[currentSprite];
    }

    int totalFrames() const { return (int)sprites.size(); }
};


#endif