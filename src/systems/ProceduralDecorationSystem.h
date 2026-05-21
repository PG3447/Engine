#pragma once

#include <vector>
#include <string>
#include <memory>
#include <random>
#include <utility>

#include <glm/glm.hpp>

class Prefab;
class GameObject;
class Scene;
class Shader;

// Jeden wpis w liście prefabów gniazda
struct WeightedPrefab {
    Prefab* prefab  = nullptr;  // wskaźnik na załadowany prefab
    float   weight  = 1.0f;    // waga losowania

    glm::vec3 localOffset = glm::vec3(0.0f);

    // zakres losowej rotacji wokół osi Y
    float   rotYMin = 0.0f;
    float   rotYMax = 360.0f;

    // zakres losowej rotacji wokół osi X i Z
    float   rotXMin = 0.0f;
    float   rotXMax = 0.0f;
    float   rotZMin = 0.0f;
    float   rotZMax = 0.0f;

    // mnożnik skali – finalScale = baseScale * losowa(scaleMin, scaleMax)
    float   scaleMin = 1.0f;
    float   scaleMax = 1.0f;
};

// Gniazdo spawnowania
struct DecorationSpawnPoint {
    // pozycja w world space
    glm::vec3  position  = glm::vec3(0.0f);

    // bazowa skala przedmiotów w tym gnieździe
    glm::vec3  baseScale = glm::vec3(1.0f);

    // opcjonalny rodzic – jeśli != nullptr, GO staje się jego dzieckiem
    GameObject* parent   = nullptr;

    // lista kandydatów – musi zawierać >= 1 wpis
    std::vector<WeightedPrefab> candidates;


    // losowa rotacja Y 0-360, skala 1:1
    DecorationSpawnPoint& Add(Prefab* prefab, float weight = 1.0f)
    {
        WeightedPrefab wp;
        wp.prefab = prefab;
        wp.weight = weight;
        candidates.push_back(wp);
        return *this;
    }


    // Pełna konfiguracja rotacji i skali
    DecorationSpawnPoint& AddFull(
        Prefab* prefab,
        float   weight,
        float   rotYMin,   float rotYMax,
        float   scaleMin = 1.0f, float scaleMax = 1.0f,
        float   rotXRange = 0.0f, float rotZRange = 0.0f)
    {
        WeightedPrefab wp;
        wp.prefab   = prefab;
        wp.weight   = weight;
        wp.rotYMin  = rotYMin;
        wp.rotYMax  = rotYMax;
        wp.rotXMin  = -rotXRange * 0.5f;
        wp.rotXMax  =  rotXRange * 0.5f;
        wp.rotZMin  = -rotZRange * 0.5f;
        wp.rotZMax  =  rotZRange * 0.5f;
        wp.scaleMin = scaleMin;
        wp.scaleMax = scaleMax;
        candidates.push_back(wp);
        return *this;
    }
};

// Wynik spawnu – przydatny do późniejszego tagowania, debug, itp.
struct SpawnedDecoration {
    GameObject*           gameObject  = nullptr;
    const WeightedPrefab* prefabEntry = nullptr; // który kandydat wygrał
};

// Główny system
class ProceduralDecorationSystem
{
public:
    // Seed dla RNG.
    // 0 (domyślnie) = losowy seed z hardware entropy każde uruchomienie.
    // Dowolna inna wartość = deterministyczny, powtarzalny układ.
    unsigned int seed = 0;

    // --- Budowanie listy gniazd ---

    void AddSpawnPoint(const DecorationSpawnPoint& sp);
    void AddSpawnPoint(DecorationSpawnPoint&& sp);

    // Wygodny helper: czyta TransformComponent + ColliderComponent stołu,
    // wylicza Y górnej powierzchni (position.y + halfSize.y) i tworzy
    // spawn pointy dla każdego z podanych offsetów XZ.
    //
    //   table      – musi mieć TransformComponent i ColliderComponent
    //   offsets    – lista pozycji XZ relative do centrum stołu
    //   candidates – pary {prefab*, weight} dodawane do każdego slotu
    //   baseScale  – bazowa skala dekoracji w tych slotach
    //
    void AddFromSurface(
        GameObject* table,
        const std::vector<glm::vec2>& offsets,
        const std::vector<std::pair<Prefab*, float>>& candidates,
        glm::vec3 baseScale = glm::vec3(1.0f));

    // --- Spawn ---
    void AddRandomlyFromSurface(
    GameObject* table,
    int count,
    const std::vector<WeightedPrefab>& candidates, // Zmieniliśmy parę na pełną strukturę WeightedPrefab
    glm::vec3 baseScale = glm::vec3(1.0f),
    float padding = 0.1f,
    float minDistance = 0.3f); // Minimalny dystans w metrach między obiektami
    // Instancjuje wszystkie zarejestrowane gniazda.
    int SpawnAll(Scene& scene, Shader* shader);

    // --- Czyszczenie ---

    // Usuwa gniazda. Nie niszczy już stworzonych GameObjectów.
    void Clear();

    // --- Wyniki ---

    const std::vector<SpawnedDecoration>& GetSpawned() const { return m_spawned; }

private:
    std::vector<DecorationSpawnPoint> m_spawnPoints;
    std::vector<SpawnedDecoration>    m_spawned;

    const WeightedPrefab* PickWeighted(
        const std::vector<WeightedPrefab>& candidates,
        std::mt19937& rng) const;
};