#ifndef COMPONENT_H
#define COMPONENT_H

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include "../unused/camera.h"

class Model;
class Shader;
class Material;

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
    std::shared_ptr<Material> materialOverride = nullptr;
};


struct RigidbodyComponent : Component {
    static constexpr uint64_t ComponentBit = 1ull << 2;

    float mass = 1.0f;
    glm::vec3 velocity{ 0.0f };
    glm::vec3 acceleration{ 0.0f };

    bool useGravity = true;
    bool isStatic = false;
};

struct Viewport {
    float x = 0.0f;
    float y = 0.0f;
    float width = 1.0f;
    float height = 1.0f;
};

struct CameraState {
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp = glm::vec3(0.0f, 1.0f, 0.0f);
};

struct CameraComponent : Component {
    static constexpr uint64_t ComponentBit = 1ull << 3;

    //Camera camera;

    CameraState state;

    //TransformComponent transform;

    float yaw = -90.0f;
    float pitch = 0.0f;

    float fov = 45.0f;
    float nearPlane = 0.1f;
    float farPlane = 10000.0f;

    //Viewport
    Viewport viewport;


    bool isActive = true;
};

struct SpriteComponent : Component {
    static constexpr uint64_t ComponentBit = 1ull << 4;

    std::vector<unsigned int> sprites;
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
        return sprites[currentSprite];
    }

    int totalFrames() const { return (int)sprites.size(); }
};


struct ColliderComponent : Component {
    static constexpr uint64_t ComponentBit = 1ull << 5;

    // offset od transformu
    glm::vec3 offset{ 0.0f, 0.0f, 0.0f };

    // połowa rozmiaru
    glm::vec3 halfSize{ 0.5f, 0.5f, 0.5f };

    bool isTrigger = false;
};


struct LightComponent : Component {
    static constexpr uint64_t ComponentBit = 1ull << 6;

    enum LightType {
        Directional = 0,
        Point = 1,
        Spot = 2
    };

    // wspólne
    int index = 0;
    bool isOn = true;
    LightType type;

    glm::vec3 ambient = glm::vec3(0.05f);
    glm::vec3 diffuse = glm::vec3(1.0f);
    glm::vec3 specular = glm::vec3(1.0f);

    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);

    // attenuation (Point / Spot)
    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;

    // Spot
    float cutOff = glm::cos(glm::radians(12.5f));
    float outerCutOff = glm::cos(glm::radians(17.5f));

};


#endif