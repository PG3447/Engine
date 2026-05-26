//
// Created by kubka on 11.05.2026.
//

#include "raycastSystem.h"

void RaycastSystem::Update(ECS&, float)
{
    struct Target { GameObject* go; glm::vec3 min, max; };
    std::vector<Target> targets;

    // Targety — wszystkie obiekty z ColliderComponent
    auto& tObjs = targetQuery_->gameobjects;
    auto& tTrs  = std::get<0>(targetQuery_->componentsVectors);
    auto& tCols = std::get<1>(targetQuery_->componentsVectors);

    for (size_t i = 0; i < tObjs.size(); i++) {
        glm::vec3 pos  = glm::vec3(tTrs[i]->modelMatrix[3]);
        glm::vec3 half = tCols[i]->halfSize * tTrs[i]->scale;
        targets.push_back({ tObjs[i], pos + tCols[i]->offset - half,
                                      pos + tCols[i]->offset + half });
    }

    for (const auto& tgt : targets) {
        DebugDrawSystem::AddAABB(tgt.min, tgt.max, glm::vec4(0, 1, 1, 1)); // cyjanowy
    }

    // Shootery — tylko obiekty z RaycastComponent
    auto& sObjs = shooterQuery_->gameobjects;
    auto& sTrs  = std::get<0>(shooterQuery_->componentsVectors);
    auto& sRcs  = std::get<1>(shooterQuery_->componentsVectors);

    for (size_t i = 0; i < sObjs.size(); i++) {
        auto* rc = sRcs[i];
        auto* tr = sTrs[i];
        rc->raycastHits.clear();

        glm::vec3 origin  = glm::vec3(tr->modelMatrix[3]) + rc->originOffset;
        //glm::vec3 forward = glm::normalize(glm::vec3(tr->modelMatrix * glm::vec4(0,0,-1,0)));

        auto* cam = sObjs[i]->GetComponent<CameraComponent>();
        glm::vec3 forward;
        if (cam != nullptr) {
            forward = glm::normalize(cam->state.Front);
        } else {
            forward = glm::normalize(glm::vec3(tr->modelMatrix * glm::vec4(0,0,-1,0)));
        }

        int count = std::max(1, rc->fovRayCount);

        for (int r = 0; r < count; ++r) {
            glm::vec3 dir = forward;
            if (count > 1) {
                float t = (float)r / (float)(count - 1);
                float angle = glm::radians(glm::mix(-rc->fovAngle * 0.5f, rc->fovAngle * 0.5f, t));
                dir = glm::normalize(glm::vec3(
                    glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0,1,0)) * glm::vec4(forward, 0)
                ));
            }

            RaycastHit best;
            best.distance = rc->range;

            for (const auto& tgt : targets) {
                if (tgt.go == sObjs[i]) continue;
                float dist = rayVsAABB(origin, dir, tgt.min, tgt.max);
                if (dist >= 0.01f && dist < best.distance) {
                    best.hit = true; best.distance = dist;
                    best.point = origin + dir * dist;
                    best.normal = aabbNormal(best.point, tgt.min, tgt.max);
                    best.hitObjectID = tgt.go->id;
                    best.hitObject = tgt.go;
                    best.hitTag = tgt.go->name;
                }
            }

            rc->raycastHits.push_back(best);
            glm::vec3 endpoint = best.hit ? best.point : origin + dir * rc->range;
            DebugDrawSystem::AddLine(origin, endpoint, best.hit ? glm::vec4(1,0,0,1) : glm::vec4(0,1,0,1));
        }
    }
}