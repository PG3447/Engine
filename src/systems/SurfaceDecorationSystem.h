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

// ─────────────────────────────────────────────────────────────────────────────
//  SurfaceDecorationCandidate
//  Jeden model dekoracyjny z wagą i zakresami transformacji
// ─────────────────────────────────────────────────────────────────────────────
struct SurfaceDecorationCandidate {
    Prefab*  prefab  = nullptr;
    char     label[64] = "";      // wyświetlana nazwa (z listy prefabów)

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

// ─────────────────────────────────────────────────────────────────────────────
//  SurfaceDecorationConfig
//  Konfiguracja dekoracji przypisana do jednego obiektu-powierzchni
// ─────────────────────────────────────────────────────────────────────────────
struct SurfaceDecorationConfig {
    char      name[64]   = "Config";
    bool      enabled    = true;

    // Obiekt z którego bierzemy górną powierzchnię kolidera
    // (wypełniany w runtime, nie serializowany — tylko nazwa)
    GameObject* targetObject = nullptr;
    char        targetName[128] = "";   // do dopasowania po nazwie przy wczytaniu

    // Ile obiektów łącznie chcemy wygenerować
    int   totalCount    = 20;

    // Minimalna odległość między obiektami (world units)
    float minDistance   = 0.5f;

    // Padding od krawędzi powierzchni
    float padding       = 0.1f;

    // ── Voronoi density ────────────────────────────────────────────────────
    // Liczba centroidów Voronoi (więcej = drobniejszy podział gęstości)
    int   voronoiPoints = 6;

    // Jak bardzo centrum komórki przyciąga obiekty (0 = płaski rozkład, 1 = silne skupienie)
    float densityFalloff = 0.6f;

    // Czy centrum obszaru ma być gęstsze (true) czy brzegi (false)
    bool  denseCenter   = true;

    // ── Skala bazowa ───────────────────────────────────────────────────────
    glm::vec3 baseScale = glm::vec3(1.0f);

    // Lista kandydatów do spawnu
    std::vector<SurfaceDecorationCandidate> candidates;
};

// ─────────────────────────────────────────────────────────────────────────────
//  SpawnedSurfaceDecoration
//  Wynik spawnu — para (GameObject*, kandydat)
// ─────────────────────────────────────────────────────────────────────────────
struct SpawnedSurfaceDecoration {
    GameObject*                        gameObject  = nullptr;
    const SurfaceDecorationCandidate*  candidate   = nullptr; // może być nullptr (np. po wczytaniu z instancji)
    std::string                        prefabLabel;            // zawsze ustawione — niezależne od candidate
    std::string                        configName;
};

// ─────────────────────────────────────────────────────────────────────────────
//  SpawnedInstanceData
//  Czysty zapis jednej zespawnowanej instancji (bez wskaźników) — do YAML.
//  Pozwala odtworzyć dokładnie ten sam układ przy starcie poziomu, bez
//  ponownego losowania.
// ─────────────────────────────────────────────────────────────────────────────
struct SpawnedInstanceData {
    std::string prefabLabel;     // nazwa prefabu (do dopasowania z availablePrefabs)
    std::string configName;      // z jakiej konfiguracji powstał (informacyjne)

    glm::vec3   position = glm::vec3(0.0f);
    glm::vec3   rotation = glm::vec3(0.0f);
    glm::vec3   scale    = glm::vec3(1.0f);
};

// ─────────────────────────────────────────────────────────────────────────────
//  SurfaceDecorationSystem
// ─────────────────────────────────────────────────────────────────────────────
class SurfaceDecorationSystem
{
public:
    // ── API konfiguracji ──────────────────────────────────────────────────
    SurfaceDecorationConfig&              AddConfig(const SurfaceDecorationConfig& cfg);
    SurfaceDecorationConfig&              AddConfig(SurfaceDecorationConfig&& cfg);
    std::vector<SurfaceDecorationConfig>& GetConfigs() { return m_configs; }
    void                                  RemoveConfig(int index);
    void                                  ClearConfigs();

    // ── Spawn ─────────────────────────────────────────────────────────────
    // Generuje dekoracje dla wszystkich aktywnych konfiguracji.
    // Zwraca łączną liczbę stworzonych obiektów.
    int SpawnAll(Scene& scene, Shader* shader);

    // Generuje dekoracje dla jednej konfiguracji.
    int SpawnConfig(const SurfaceDecorationConfig& cfg, Scene& scene, Shader* shader);

    // Usuwa wszystkie wcześniej wygenerowane obiekty ze sceny
    void DespawnAll(Scene& scene);

    // ── Wyniki ────────────────────────────────────────────────────────────
    const std::vector<SpawnedSurfaceDecoration>& GetSpawned() const { return m_spawned; }

    // ── ImGui ─────────────────────────────────────────────────────────────
    // availablePrefabs: lista (nazwa, Prefab*) załadowanych modeli
    // sceneObjects:     lista wszystkich GameObject* ze sceny (do wyboru powierzchni)
    // Zwraca true jeśli cokolwiek się zmieniło.
    bool DrawImGui(
        const std::vector<std::pair<std::string, Prefab*>>& availablePrefabs,
        const std::vector<GameObject*>&                     sceneObjects,
        Scene&                                              scene,
        Shader*                                             shader);

    // ── YAML (konfiguracje edytora) ──────────────────────────────────────
    bool SaveToYaml(const std::string& path) const;
    bool LoadFromYaml(const std::string& path,
                      const std::vector<std::pair<std::string, Prefab*>>& availablePrefabs,
                      const std::vector<GameObject*>&                     sceneObjects);

    // ── YAML (zespawnowane instancje — runtime, do wczytania na starcie poziomu) ──
    // Zapisuje dokładne pozycje/rotacje/skale wszystkich aktualnie zespawnowanych
    // obiektów (m_spawned). Wywołaj po SpawnAll(), gdy jesteś zadowolony z układu.
    bool SaveInstancesToYaml(const std::string& path) const;

    // Wczytuje zapisane instancje i tworzy obiekty na scenie z dokładnymi
    // transformacjami z pliku — bez ponownego losowania/Voronoi.
    // Użyj tego przy starcie poziomu zamiast SpawnAll().
    // Zwraca liczbę utworzonych obiektów.
    int LoadInstancesFromYaml(
        const std::string& path,
        const std::vector<std::pair<std::string, Prefab*>>& availablePrefabs,
        Scene& scene, Shader* shader);

private:
    std::vector<SurfaceDecorationConfig>    m_configs;
    std::vector<SpawnedSurfaceDecoration>   m_spawned;

    // ── Voronoi density sampling ──────────────────────────────────────────
    // Zwraca znormalizowany "weight" dla punktu (px,pz) w obszarze
    // na podstawie Voronoi centroidów. Im bliżej centroidu tym wyższy weight.
    float VoronoiDensityWeight(
        float px, float pz,
        float originX, float originZ,
        float width, float depth,
        const std::vector<glm::vec2>& centroids,
        float falloff,
        bool  denseCenter) const;

    // Generuje deterministyczne centroidy Voronoi dla danej konfiguracji
    std::vector<glm::vec2> GenerateVoronoiCentroids(
        float originX, float originZ,
        float width, float depth,
        int   count) const;

    // Rejection sampling z Voronoi density
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

    // Losuje kandydata ważonym losowaniem
    const SurfaceDecorationCandidate* PickWeighted(
        const std::vector<SurfaceDecorationCandidate>& candidates) const;

    // Losuje float z zakresu [lo, hi]
    static float RandRange(float lo, float hi);

    // ── ImGui helpers ─────────────────────────────────────────────────────
    bool DrawConfigEditor(
        SurfaceDecorationConfig&                            cfg,
        const std::vector<std::pair<std::string, Prefab*>>& availablePrefabs,
        const std::vector<GameObject*>&                     sceneObjects,
        int                                                 cfgIndex);

    // Indeks aktualnie wybranej konfiguracji w ImGui (-1 = żadna)
    int  m_selectedConfig = -1;

    // Ścieżka pliku YAML (przechowywana między klatkami dla pola tekstowego)
    char m_yamlPath[256]  = "res/surface_decorations.yaml";
    char m_instancesYamlPath[256] = "res/surface_decorations_instances.yaml";
};