#ifndef GENERATIVE_SYSTEM_H
#define GENERATIVE_SYSTEM_H

#include "core/system.h"
#include "core/ecs.h"
#include "core/component.h"
#include "utils/random_helper.h"
#include "prefab.h"
#include "core/scene.h"
#include <glm/gtc/constants.hpp>
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>

class GenerativeSystem : public System {
private:
    Shader* defaultShader;

public:
    GenerativeSystem(ECS& ecs, Shader* shader) : defaultShader(shader) {}

    void OnGameObjectUpdated(GameObject* e) override {}

    void Update(ECS& ecs, float dt) override {
        auto objects = ecs.GetAllGameObjects();

        for (auto* obj : objects) {
            if ((obj->componentMask & GenerativeComponent::ComponentBit) == GenerativeComponent::ComponentBit) {

                auto* genComp = obj->GetComponent<GenerativeComponent>();

                if (!genComp || genComp->hasSpawned || genComp->prefabs.empty() || !genComp->targetScene) {
                    continue;
                }

                std::vector<glm::vec3> placedPositions;
                for (auto* child : obj->GetChildren()) {
                    if (auto* t = child->GetComponent<TransformComponent>()) {
                        placedPositions.push_back(t->position);
                    }
                }

                for (int i = 0; i < genComp->spawnCount; ++i) {
                    int prefabIndex = RandomHelper::RangeInt(0, (int)genComp->prefabs.size() - 1);
                    auto selectedPrefab = genComp->prefabs[prefabIndex];

                    if (!selectedPrefab) {
                        spdlog::warn("GenerativeSystem: selectedPrefab null - skipping");
                        continue;
                    }

                    glm::vec3 localPos(0.0f);
                    bool placed = false;
                    for (int attempt = 0; attempt < genComp->maxPlacementAttempts && !placed; ++attempt) {
                        if (genComp->shape == GenerativeShape::Circle) {
                            float angle = RandomHelper::Range(0.0f, 2.0f * glm::pi<float>());
                            float r = RandomHelper::Range(0.0f, genComp->extents.x);
                            localPos.x = r * std::cos(angle);
                            localPos.z = r * std::sin(angle);
                        } else { // Box
                            localPos.x = RandomHelper::Range(-genComp->extents.x / 2.0f, genComp->extents.x / 2.0f);
                            localPos.z = RandomHelper::Range(-genComp->extents.y / 2.0f, genComp->extents.y / 2.0f);
                        }

                        bool ok = true;
                        for (auto& p : placedPositions) {
                            const float dx = p.x - localPos.x;
                            const float dz = p.z - localPos.z;
                            const float dist2 = dx * dx + dz * dz;
                            if (dist2 < (genComp->minSpacing * genComp->minSpacing)) {
                                ok = false;
                                break;
                            }
                        }
                        if (ok) placed = true;
                    }

                    if (!placed) {
                        spdlog::debug("GenerativeSystem: couldn't place object (index {}) after {} attempts", i, genComp->maxPlacementAttempts);
                        continue;
                    }

                    GameObject* newObj = selectedPrefab->Instantiate(*(genComp->targetScene), obj, defaultShader);
                    if (!newObj) {
                        spdlog::warn("GenerativeSystem: Instantiate returned nullptr");
                        continue;
                    }

                    auto* newTransform = newObj->GetComponent<TransformComponent>();
                    if (newTransform) {
                        newTransform->position = localPos;

                        float sx = RandomHelper::Range(genComp->minScale.x, genComp->maxScale.x) * genComp->globalScaleMultiplier;
                        float sy = RandomHelper::Range(genComp->minScale.y, genComp->maxScale.y) * genComp->globalScaleMultiplier;
                        float sz = RandomHelper::Range(genComp->minScale.z, genComp->maxScale.z) * genComp->globalScaleMultiplier;
                        newTransform->scale = glm::vec3(sx, sy, sz);

                        if (genComp->randomizeYRotation) {
                            newTransform->rotation.y = RandomHelper::Range(0.0f, 360.0f);
                        }
                        newTransform->isDirty = true;
                    }

                    if (genComp->addColliders) {
                        auto* col = newObj->AddComponent<ColliderComponent>();
                        glm::vec3 base = genComp->colliderHalfSize;
                        glm::vec3 scale = newTransform ? newTransform->scale : glm::vec3(1.0f);
                        float maxS = glm::max(glm::max(scale.x, scale.y), scale.z);
                        col->halfSize = base * maxS;
                        col->offset = glm::vec3(0.0f);
                    }

                    newObj->AddComponent<GeneratedComponent>();

                    placedPositions.push_back(localPos);
                }

                genComp->hasSpawned = true;
            }
        }
    }
};

#endif