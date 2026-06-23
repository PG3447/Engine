#pragma once

#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <optional>
#include <unordered_map>

#include <glm/glm.hpp>

class Prefab;
class GameObject;
class Scene;
class Shader;

struct SurfaceDecorationCandidate {
    Prefab*  prefab  = nullptr;
    char     label[64] = "";      // wyświetlana nazwa

    float    weight   = 1.0f;     // waga losowania

    float    scaleMin = 1.0f;
    float    scaleMax = 1.0f;

    float    rotYMin  = 0.0f;
    float    rotYMax  = 360.0f;
    float    rotXMin  = 0.0f;
    float    rotXMax  = 0.0f;
    float    rotZMin  = 0.0f;
    float    rotZMax  = 0.0f;

    glm::vec3 localOffset = glm::vec3(0.0f);
};

struct SurfaceDecorationConfig {
    char      name[64]   = "Config";
    bool      enabled    = true;

    GameObject* targetObject = nullptr;
    char        targetName[128] = "";

    int   totalCount    = 20;

    float minDistance   = 0.5f;

    float padding       = 0.1f;

    int   voronoiPoints = 6;

    float densityFalloff = 0.6f;

    bool  denseCenter   = true;

    glm::vec3 baseScale = glm::vec3(1.0f);

    std::vector<SurfaceDecorationCandidate> candidates;
};


struct SpawnedSurfaceDecoration {
    GameObject*                        gameObject  = nullptr;
    const SurfaceDecorationCandidate*  candidate   = nullptr;
    std::string                        prefabLabel;
    std::string                        configName;
};

struct SpawnedInstanceData {
    std::string prefabLabel;     // nazwa prefabu
    std::string configName;      // z jakiej konfiguracji powstał

    glm::vec3   position = glm::vec3(0.0f);
    glm::vec3   rotation = glm::vec3(0.0f);
    glm::vec3   scale    = glm::vec3(1.0f);
};

class SurfaceDecorationSystem
{
public:
    SurfaceDecorationConfig&              AddConfig(const SurfaceDecorationConfig& cfg);
    SurfaceDecorationConfig&              AddConfig(SurfaceDecorationConfig&& cfg);
    std::vector<SurfaceDecorationConfig>& GetConfigs() { return m_configs; }
    void                                  RemoveConfig(int index);
    void                                  ClearConfigs();

    int SpawnAll(Scene& scene, Shader* shader);

    // Generuje dekoracje dla jednej konfiguracji.
    int SpawnConfig(const SurfaceDecorationConfig& cfg, Scene& scene, Shader* shader);

    // Usuwa wszystkie wcześniej wygenerowane obiekty ze sceny
    void DespawnAll(Scene& scene);

    const std::vector<SpawnedSurfaceDecoration>& GetSpawned() const { return m_spawned; }

    bool DrawImGui(
        const std::vector<std::pair<std::string, Prefab*>>& availablePrefabs,
        const std::vector<GameObject*>&                     sceneObjects,
        Scene&                                              scene,
        Shader*                                             shader);

    bool SaveToYaml(const std::string& path) const;
    bool LoadFromYaml(const std::string& path,
                      const std::vector<std::pair<std::string, Prefab*>>& availablePrefabs,
                      const std::vector<GameObject*>&                     sceneObjects);

    bool SaveInstancesToYaml(const std::string& path) const;

    int LoadInstancesFromYaml(
        const std::string& path,
        const std::vector<std::pair<std::string, Prefab*>>& availablePrefabs,
        Scene& scene, Shader* shader);

private:
    std::vector<SurfaceDecorationConfig>    m_configs;
    std::vector<SpawnedSurfaceDecoration>   m_spawned;

    float VoronoiDensityWeight(
        float px, float pz,
        float originX, float originZ,
        float width, float depth,
        const std::vector<glm::vec2>& centroids,
        float falloff,
        bool  denseCenter) const;

    std::vector<glm::vec2> GenerateVoronoiCentroids(
        float originX, float originZ,
        float width, float depth,
        int   count) const;

    std::optional<glm::vec2> SamplePoint(
        float originX, float originZ,
        float width, float depth,
        float padding,
        float minDistance,
        const std::vector<glm::vec2>& placed,
        const std::vector<glm::vec2>& centroids,
        float falloff,
        bool  denseCenter,
        int   maxAttempts = 80) const;

    const SurfaceDecorationCandidate* PickWeighted(
        const std::vector<SurfaceDecorationCandidate>& candidates) const;

    static float RandRange(float lo, float hi);

    bool DrawConfigEditor(
        SurfaceDecorationConfig&                            cfg,
        const std::vector<std::pair<std::string, Prefab*>>& availablePrefabs,
        const std::vector<GameObject*>&                     sceneObjects,
        int                                                 cfgIndex);

    int  m_selectedConfig = -1;

    char m_yamlPath[256]  = "res/surface_decorations.yaml";
    char m_instancesYamlPath[256] = "res/surface_decorations_instances.yaml";
};