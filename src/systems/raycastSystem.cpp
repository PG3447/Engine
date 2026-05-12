//
// Created by kubka on 11.05.2026.
//

#include "raycastSystem.h"

void RaycastSystem::Update(ECS&, float)
{
    const auto& objects = query_->gameobjects;

    struct Target {
        GameObject* go;
        glm::vec3 min;
        glm::vec3 max;
    };

    std::vector<Target> targets;
    targets.reserve(objects.size());

    // Zbieranie colliderów
    for (auto* go : objects) {
        auto* col = go->GetComponent<ColliderComponent>();
        auto* tr  = go->GetComponent<TransformComponent>();

        glm::vec3 pos  = glm::vec3(tr->modelMatrix[3]);
        glm::vec3 half = col->halfSize * tr->scale;

        targets.push_back({
            go,
            pos + col->offset - half,
            pos + col->offset + half
        });
    }

    // Raycast
    for (auto* go : objects) {
        auto* rc = go->GetComponent<RaycastComponent>();
        auto* tr = go->GetComponent<TransformComponent>();

        rc->raycastHits.clear();

        glm::vec3 origin =
            glm::vec3(tr->modelMatrix[3]) + rc->originOffset;

        glm::vec3 forward = glm::normalize(
            glm::vec3(tr->modelMatrix * glm::vec4(0, 0, -1, 0))
        );

        int count = std::max(1, rc->fovRayCount);

        for (int i = 0; i < count; ++i) {
            glm::vec3 dir;

            if (count == 1) {
                dir = forward;
            } else {
                float t = (float)i / (float)(count - 1);       // 0.0 .. 1.0
                float angle = glm::radians(
                    glm::mix(-rc->fovAngle * 0.5f, rc->fovAngle * 0.5f, t)
                );
                glm::vec3 right = glm::normalize(
                    glm::vec3(tr->modelMatrix * glm::vec4(1, 0, 0, 0))
                );
                dir = glm::normalize(
                    glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0,1,0))
                    * glm::vec4(forward, 0)
                );
            }
            RaycastHit best;
            best.distance = rc->range;

            for (const auto& t : targets) {
                if (t.go == go) continue;

                float dist = rayVsAABB(origin, dir, t.min, t.max);
                if (dist >= 0.0f && dist < best.distance) {
                    best.hit         = true;
                    best.distance    = dist;
                    best.point       = origin + dir * dist;
                    best.normal      = aabbNormal(best.point, t.min, t.max);
                    best.hitObjectID = t.go->id;
                }
            }

            rc->raycastHits.push_back(best);
            // Debug draw
            glm::vec3 endpoint = best.hit
                ? best.point
                : origin + dir * rc->range;

            glm::vec4 color = best.hit
                ? glm::vec4(1, 0, 0, 1)   // czerwony = trafił
                : glm::vec4(0, 1, 0, 1);  // zielony = nie trafił

            DebugDrawSystem::AddLine(origin, endpoint, color);
        }

    }

}