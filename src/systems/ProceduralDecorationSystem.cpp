#include "ProceduralDecorationSystem.h"

#include <prefab.h>
#include <core/scene.h>
#include "core/gameobject.h"
#include <core/component.h>
#include <shader.h>

#include <glm/gtc/constants.hpp>
#include <spdlog/spdlog.h>

namespace {
    float toRad(float deg) { return deg * glm::pi<float>() / 180.0f; }

    float randRange(std::mt19937& rng, float lo, float hi)
    {
        if (lo >= hi) return lo;
        std::uniform_real_distribution<float> dist(lo, hi);
        return dist(rng);
    }
}

void ProceduralDecorationSystem::AddSpawnPoint(const DecorationSpawnPoint& sp)
{
    m_spawnPoints.push_back(sp);
}

void ProceduralDecorationSystem::AddSpawnPoint(DecorationSpawnPoint&& sp)
{
    m_spawnPoints.push_back(std::move(sp));
}

void ProceduralDecorationSystem::AddFromSurface(
    GameObject* table,
    const std::vector<glm::vec2>& offsets,
    const std::vector<std::pair<Prefab*, float>>& candidates,
    glm::vec3 baseScale)
{
    if (!table) {
        spdlog::warn("[ProceduralDecorationSystem] AddFromSurface: table is null, skipped.");
        return;
    }

    auto* tr  = table->GetComponent<TransformComponent>();
    auto* col = table->GetComponent<ColliderComponent>();

    if (!tr || !col) {
        spdlog::warn("[ProceduralDecorationSystem] AddFromSurface: table '{}' missing "
                     "TransformComponent or ColliderComponent, skipped.", table->name);
        return;
    }

    // Górna powierzchnia blatu w world space.
    float surfaceY = tr->position.y + col->halfSize.y;

    for (const auto& off : offsets)
    {
        DecorationSpawnPoint sp;
        sp.position  = glm::vec3(
            tr->position.x + col->offset.x + off.x,
            surfaceY,
            tr->position.z + col->offset.z + off.y
        );
        sp.baseScale = baseScale;
        sp.parent    = nullptr;

        for (const auto& [prefab, weight] : candidates)
            sp.Add(prefab, weight);

        m_spawnPoints.push_back(std::move(sp));
    }
}

void ProceduralDecorationSystem::Clear()
{
    m_spawnPoints.clear();
}

int ProceduralDecorationSystem::SpawnAll(Scene& scene, Shader* shader)
{
    std::mt19937 rng;
    if (seed == 0) {
        std::random_device rd;
        rng.seed(rd());
    } else {
        rng.seed(seed);
    }

    int created = 0;

    for (const auto& sp : m_spawnPoints)
    {
        if (sp.candidates.empty()) {
            spdlog::warn("[ProceduralDecorationSystem] Spawn point at ({:.1f},{:.1f},{:.1f}) "
                         "has no candidates – skipped.",
                         sp.position.x, sp.position.y, sp.position.z);
            continue;
        }

        const WeightedPrefab* chosen = PickWeighted(sp.candidates, rng);
        if (!chosen || !chosen->prefab) {
            spdlog::warn("[ProceduralDecorationSystem] PickWeighted returned null – skipped.");
            continue;
        }

        GameObject* go = chosen->prefab->Instantiate(scene, sp.parent, shader);
        if (!go) {
            spdlog::error("[ProceduralDecorationSystem] Prefab::Instantiate returned null.");
            continue;
        }

        auto* tr = go->GetComponent<TransformComponent>();
        if (tr)
        {
            tr->position = sp.position + chosen->localOffset;

            tr->rotation = glm::vec3(
                randRange(rng, chosen->rotXMin, chosen->rotXMax),
                randRange(rng, chosen->rotYMin, chosen->rotYMax),
                randRange(rng, chosen->rotZMin, chosen->rotZMax)
            );

            float scaleMult = randRange(rng, chosen->scaleMin, chosen->scaleMax);
            tr->scale    = sp.baseScale * scaleMult;
            tr->isDirty  = true;
        }

        m_spawned.push_back({ go, chosen });
        ++created;
    }

    spdlog::info("[ProceduralDecorationSystem] SpawnAll() – {} obiektów stworzonych.", created);
    return created;
}

const WeightedPrefab* ProceduralDecorationSystem::PickWeighted(
    const std::vector<WeightedPrefab>& candidates,
    std::mt19937& rng) const
{
    if (candidates.empty()) return nullptr;

    float total = 0.0f;
    for (const auto& c : candidates)
        total += c.weight;

    if (total <= 0.0f) {
        // Wszystkie wagi 0 → wybór uniform, nie crashujemy
        std::uniform_int_distribution<size_t> pick(0, candidates.size() - 1);
        return &candidates[pick(rng)];
    }

    std::uniform_real_distribution<float> dist(0.0f, total);
    float roll = dist(rng);

    float cumulative = 0.0f;
    for (const auto& c : candidates) {
        cumulative += c.weight;
        if (roll <= cumulative)
            return &c;
    }

    return &candidates.back(); // floating point safety
}
void ProceduralDecorationSystem::AddRandomlyFromSurface(
    GameObject* table,
    int count,
    const std::vector<WeightedPrefab>& candidates,
    glm::vec3 baseScale,
    float padding,
    float minDistance)
{
    if (!table || candidates.empty()) {
        spdlog::warn("[ProceduralDecorationSystem] AddRandomlyFromSurface: Invalid arguments, skipped.");
        return;
    }

    auto* tr  = table->GetComponent<TransformComponent>();
    auto* col = table->GetComponent<ColliderComponent>();

    if (!tr || !col) {
        spdlog::warn("[ProceduralDecorationSystem] AddRandomlyFromSurface: table '{}' missing components.", table->name);
        return;
    }

    std::mt19937 rng;
    if (seed == 0) {
        std::random_device rd;
        rng.seed(rd());
    } else {
        rng.seed(seed);
    }

    float surfaceY = tr->position.y + col->halfSize.y;

    float minX = col->offset.x - col->halfSize.x + padding;
    float maxX = col->offset.x + col->halfSize.x - padding;
    float minZ = col->offset.z - col->halfSize.z + padding;
    float maxZ = col->offset.z + col->halfSize.z - padding;

    if (minX > maxX) std::swap(minX, maxX);
    if (minZ > maxZ) std::swap(minZ, maxZ);

    std::uniform_real_distribution<float> distX(minX, maxX);
    std::uniform_real_distribution<float> distZ(minZ, maxZ);

    std::vector<glm::vec3> localTablePoints;

    const int MAX_ATTEMPTS = 50;

    for (int i = 0; i < count; ++i)
    {
        glm::vec3 chosenPosition(0.0f);
        bool foundValidPoint = false;

        for (int attempt = 0; attempt < MAX_ATTEMPTS; ++attempt)
        {
            float randomX = distX(rng);
            float randomZ = distZ(rng);

            glm::vec3 candidatePos = glm::vec3(
                tr->position.x + randomX,
                surfaceY,
                tr->position.z + randomZ
            );

            // Sprawdzamy, czy punkt nie jest za blisko innych obiektów na tym stole
            bool tooClose = false;
            for (const auto& existingPos : localTablePoints)
            {
                // Ignorujemy wysokość Y (sprawdzamy dystans tylko w rzucie z góry XZ)
                float distXZ = glm::distance(glm::vec2(candidatePos.x, candidatePos.z),
                                             glm::vec2(existingPos.x, existingPos.z));
                if (distXZ < minDistance)
                {
                    tooClose = true;
                    break;
                }
            }

            if (!tooClose)
            {
                chosenPosition = candidatePos;
                foundValidPoint = true;
                break;
            }
        }

        if (!foundValidPoint)
        {
            spdlog::warn("[ProceduralDecorationSystem] Mogło zabraknąć miejsca na stole '{}' dla obiektu nr {}. Pomijam punkt.", table->name, i);
            continue;
        }

        // Zapisujemy punkt do lokalnej listy walidacyjnej
        localTablePoints.push_back(chosenPosition);

        // Tworzymy punkt spawnu
        DecorationSpawnPoint sp;
        sp.position  = chosenPosition;
        sp.baseScale = baseScale;
        sp.parent    = nullptr;

        // Kopiujemy całą listę kandydatów wraz z ich unikalnymi zakresami rotacji/skali
        sp.candidates = candidates;

        m_spawnPoints.push_back(std::move(sp));
    }
}