#ifndef COMPONENT_H
#define COMPONENT_H

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include "../unused/camera.h"
#include "yaml_config.h"
#include "mesh_data.h"

class GameObject;
class MeshNode;
class Shader;
class Material;
class Skeleton;

struct AnimatorComponent;
struct AnimationChannel;
struct AnimationClip;

struct NodeAnimCache {
    const AnimationChannel* channel = nullptr;
    bool isBone = false;
    int boneID = -1;
    glm::mat4 offset{ 1.0f };

    int lastPosIndex = 0;
    int lastRotIndex = 0;
    int lastScaleIndex = 0;
};

struct Component {
    static constexpr bool Unique = false;
    virtual ~Component() {}

    virtual void OnEnable(GameObject* owner) {}
    virtual void OnDisable(GameObject* owner) {}

    virtual const char* GetTypeName() const { return "Component"; }

    virtual void Serialize(YAML::Node& node) {}
    virtual void Deserialize(const YAML::Node& node) {}
};


struct TransformComponent : Component {
    static constexpr uint64_t ComponentBit = 1ull << 0;

    glm::vec3 position{ 0.0f, 0.0f, 0.0f };
    glm::vec3 rotation{ 0.0f, 0.0f, 0.0f };
    glm::vec3 scale{ 1.0f, 1.0f, 1.0f };

    glm::mat4 modelMatrix{ 1.0f };

    bool isDirty = true;

    const char* GetTypeName() const override { return "Transform"; }

    void Serialize(YAML::Node& node) override
    {
        node["position"] = position;
        node["rotation"] = rotation;
        node["scale"] = scale;
    }

    void Deserialize(const YAML::Node& node) override
    {
        if (node["position"])
            position = node["position"].as<glm::vec3>();

        if (node["rotation"])
            rotation = node["rotation"].as<glm::vec3>();

        if (node["scale"])
            scale = node["scale"].as<glm::vec3>();

        isDirty = true;
    }
};

//struct ModelNode
//{
//    // ===== SCENE HIERARCHY =====
//    std::string name;
//
//    Transform transform;
//
//    std::vector<std::unique_ptr<ModelNode>> children;
//
//    // ===== GEOMETRY =====
//    std::vector<MeshNode> meshes;
//};
//
//struct MeshNode
//{
//    std::shared_ptr<RenderMesh> mesh;     // GPU geometry
//    std::shared_ptr<Material> material;   // shading data
//};

struct RenderComponent : Component {
    static constexpr uint64_t ComponentBit = 1ull << 1;

    std::vector<MeshNode> meshes;
    AnimatorComponent* animator;
    AABB localObjectAABB;


    //std::vector<std::shared_ptr<Material>> materials; Fajnie jak bedzie xD

    //std::shared_ptr<Model> model;
    //
    //
    //std::shared_ptr<ModelNode> node;
    //Model* model = nullptr;
    //Shader* shader = nullptr;
    //std::shared_ptr<Material> materialOverride = nullptr;
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
    glm::vec3 Front = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 Right = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 WorldUp = glm::vec3(0.0f, 1.0f, 0.0f);
};

struct CameraComponent : Component {
    static constexpr uint64_t ComponentBit = 1ull << 3;

    //Camera camera;

    CameraState state;

    //TransformComponent transform;

  /*  float yaw = -90.0f;
    float pitch = 0.0f;*/

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
    glm::vec3 textColor = { 255.0f, 255.0f, 255.0f };
    glm::vec2 textOffset = { 0.0f, 0.0f };
    bool textOutlineEnabled = false;
    float textOutlineSize = 1.5f;
    glm::vec3 textOutlineColor = { 0.0f, 0.0f, 0.0f };
    bool textCentered = true;

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
    bool affectsNavMesh = false;
    bool isWalkable = false;
    
    const char* GetTypeName() const override { return "Collider"; }

    void Serialize(YAML::Node& node) override
    {
        node["offset"] = offset;
        node["halfSize"] = halfSize;
        node["isTrigger"] = isTrigger;
    }

    void Deserialize(const YAML::Node& node) override
    {
        if (node["offset"])
            offset = node["offset"].as<glm::vec3>();

        if (node["halfSize"])
            halfSize = node["halfSize"].as<glm::vec3>();

        if (node["isTrigger"])
            isTrigger = node["isTrigger"].as<bool>();
    }

};

enum LightType {
    Directional = 0,
    Point = 1,
    Spot = 2
};

struct LightComponent : Component {
    static constexpr uint64_t ComponentBit = 1ull << 6;

    // wspólne
    int index = 0;
    bool isOn = true;
    LightType type;

    glm::vec3 ambient = glm::vec3(0.05f);
    glm::vec3 diffuse = glm::vec3(1.0f);
    glm::vec3 specular = glm::vec3(1.0f);

    //glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 direction = glm::vec3(0.0f);

    // attenuation (Point / Spot)
    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;

    // Spot
    float cutOff = glm::cos(glm::radians(12.5f));
    float outerCutOff = glm::cos(glm::radians(17.5f));

    const char* GetTypeName() const override { return "Light"; }

    void Serialize(YAML::Node& node) override
    {
        node["type"] = "Light";

        node["index"] = index;
        node["isOn"] = isOn;

        node["lightType"] = static_cast<int>(type);

        node["ambient"] = ambient;
        node["diffuse"] = diffuse;
        node["specular"] = specular;

        node["direction"] = direction;

        node["constant"] = constant;
        node["linear"] = linear;
        node["quadratic"] = quadratic;

        node["cutOff"] = cutOff;
        node["outerCutOff"] = outerCutOff;
    }


    void Deserialize(const YAML::Node& node) override
    {
        if (node["index"])
            index = node["index"].as<int>();

        if (node["isOn"])
            isOn = node["isOn"].as<bool>();

        if (node["lightType"])
            type = static_cast<LightType>(
                node["lightType"].as<int>()
                );

        if (node["ambient"])
            ambient = node["ambient"].as<glm::vec3>();

        if (node["diffuse"])
            diffuse = node["diffuse"].as<glm::vec3>();

        if (node["specular"])
            specular = node["specular"].as<glm::vec3>();

        if (node["direction"])
            direction = node["direction"].as<glm::vec3>();

        if (node["constant"])
            constant = node["constant"].as<float>();

        if (node["linear"])
            linear = node["linear"].as<float>();

        if (node["quadratic"])
            quadratic = node["quadratic"].as<float>();

        if (node["cutOff"])
            cutOff = node["cutOff"].as<float>();

        if (node["outerCutOff"])
            outerCutOff = node["outerCutOff"].as<float>();
    }
};

struct AnimatorComponent : Component {
    static constexpr uint64_t ComponentBit = 1ull << 7;
    static constexpr bool Unique = true;

    static const int MAX_BONES = 200;

    Skeleton* currentSkeleton = nullptr;
    AnimationClip* currentAnimation = nullptr;
    float currentTime = 0.0f;
    float playbackSpeed = 1.0f;
    bool looping = true;
    bool isFinished = false;

    std::vector<NodeAnimCache> animCache;

    std::vector<glm::mat4> finalBoneMatrices;

    AnimatorComponent()
    {
        finalBoneMatrices.resize(MAX_BONES, glm::mat4(1.0f));
    }

    void OnEnable(GameObject* owner) override;
};

struct RaycastHit {
    bool hit = false;
    float distance = 0.0f; //odleglosc
    glm::vec3 point = {}; //punkt trafienia
    glm::vec3 normal = {}; //normalna od trafionej sciany AABB
    size_t hitObjectID = SIZE_MAX; //id trafionego GameObjectu (Size max to brak)
    GameObject* hitObject = nullptr;
    std::string hitTag = "";
};

struct RaycastComponent : Component {
    static constexpr uint64_t ComponentBit = 1ull << 8;

    //wartosci domyslne
    float range = 50.0f; // zasieg widzenia
    int fovRayCount = 1;
    float fovAngle = 90.0f; //kat luku w stopniach
    glm::vec3 originOffset = glm::vec3(0.0f, 0.0f, 0.0f); //przesuniece od zrodla pozycji

    std::vector<RaycastHit> raycastHits;

    //Czy w klatce promien trafil w cokolwiek
    bool anyHit() const {
        for (const auto & h : raycastHits) {
            if (h.hit) {
                return true;
            }
        }
        return false;
    }

    RaycastHit closestHit() const {
        RaycastHit bestHit;
        bestHit.distance = 1e30f;
        for (const auto & h : raycastHits) {
            if (h.hit && h.distance < bestHit.distance) {
                bestHit = h;
            }
        }
        return (bestHit.distance < 1e30f) ? bestHit : RaycastHit{};
    }

    //debug
    bool debugDraw = false;
    glm::vec4 colorMiss = {0.0f, 1.0f, 0.0f, 1.0f}; // zielony  = brak trafienia
    glm::vec4 colorHit  = {1.0f, 0.3f, 0.0f, 1.0f}; // pomarańczowy = trafienie

};
struct NavVertex {
    glm::vec3 position;
};

// Trojkat siatki nawigacyjnej
struct NavTriangle {
    int v[3];
    int neighbors[3];

    glm::vec3 centroid;
    bool walkable = true;

    NavTriangle() {
        v[0] = v[1] = v[2] = -1;
        neighbors[0] = neighbors[1] = neighbors[2] = -1;
        centroid = glm::vec3(0.0f);
    }
    NavTriangle(int a, int b, int c) {
        v[0] = a; v[1] = b; v[2] = c;
        neighbors[0] = neighbors[1] = neighbors[2] = -1;
        centroid = glm::vec3(0.0f);
    }
};
// Zbior danych wynikowych po bake
struct NavMeshData {
    std::vector<NavVertex>   vertices;
    std::vector<NavTriangle> triangles;
    bool isBaked = false;

    void Clear() {
        vertices.clear();
        triangles.clear();
        isBaked = false;
    }
};
struct NavMeshComponent : Component {
    static constexpr uint64_t ComponentBit = 1ull << 10;

    NavMeshData data;

    // Parametry bake
    float agentRadius   = 0.5f;   // Promien agenta (margines przy przeszkodach)
    float agentHeight   = 2.0f;   // Wysokosc agenta
    float voxelSize     = 5.0f;   // Rozmiar voksela dla siatki punktow probkowania
    float maxSlopeAngle = 45.0f;  // Max kat nachylenia (w stopniach) - powyzej = niechodzalne

    // Debug
    bool debugDraw = true;
    glm::vec4 colorWalkable    = glm::vec4(0.0f, 0.8f, 0.2f, 0.4f); // zielony
    glm::vec4 colorUnwalkable  = glm::vec4(0.8f, 0.1f, 0.1f, 0.4f); // czerwony
    glm::vec4 colorEdge        = glm::vec4(0.0f, 1.0f, 0.5f, 1.0f); // jasny zielony

    // Znajdz trojkat zawierajacy punkt (XZ), zwraca indeks lub -1
    int FindTriangle(const glm::vec3& worldPos) const;

    // Czy punkt jest na navmeshu
    bool IsPointWalkable(const glm::vec3& worldPos) const;
};
enum class NavAgentState {
    Idle,
    RequestingPath,
    Moving,
    Arrived,
};

struct NavPathComponent : Component {
    static constexpr uint64_t ComponentBit = 1ull << 11;

    // --- Cel i sciezka ---
    glm::vec3 goalPosition{ 0.0f };
    std::vector<glm::vec3> path;   // Punkty wygladzonej sciezki (wynik funnela)
    int currentWaypoint = 0;       // Indeks aktualnego punktu docelowego w path

    // --- Parametry ruchu ---
    float moveSpeed        = 5.0f;
    float waypointRadius   = 0.5f; // Jak blisko punktu zeby uznac ze dotarl
    float arrivalRadius    = 1.0f; // Jak blisko celu zeby uznac ze dotarl do konca

    // --- Czas oczekiwania po dotarciu ---
    float idleTimeMin  = 0.5f;
    float idleTimeMax  = 2.0f;
    float idleTimer    = 0.0f;

    // --- Stan ---
    NavAgentState state = NavAgentState::Idle;

    // --- Debug ---
    bool debugDraw = true;
    glm::vec4 colorPath    = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f); // zolty
    glm::vec4 colorGoal    = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f); // czerwony
    glm::vec4 colorWaypoint= glm::vec4(0.0f, 0.5f, 1.0f, 1.0f); // niebieski
};
#endif