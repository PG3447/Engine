//
// Created by kubka on 11.05.2026.
//

#ifndef MIMICRY_EXPERIMENTS_RAYCASTSYSTEM_H
#define MIMICRY_EXPERIMENTS_RAYCASTSYSTEM_H

#include "core/ecs.h"
#include "core/scene.h"
#include "core/gameobject.h"
#include "core/component.h"
#include "systems/HID.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <imgui.h>
#include <spdlog/spdlog.h>
#include <algorithm>



class RaycastSystem : public System {
public:
    RaycastSystem(ECS& ecs)
            : ecs_(ecs)
    {
        shooterQuery_ = ecs_.CreateQuery<TransformComponent, RaycastComponent>();
        targetQuery_  = ecs_.CreateQuery<TransformComponent, ColliderComponent>();
    }
    void OnGameObjectUpdated(GameObject* gameObject) override {}
    void Update(ECS& ecs, float dt) override;
private:
    ECS& ecs_;
    Query<TransformComponent, RaycastComponent>*  shooterQuery_;
    Query<TransformComponent, ColliderComponent>* targetQuery_;
    static glm::vec3 rotateY(const glm::vec3& dir, float angleDeg) {
        float r = glm::radians(angleDeg);
        float c = glm::cos(r);
        float s = glm::sin(r);
        return {
            dir.x * c + dir.z * s,
            dir.y,
            -dir.x * s + dir.z * c
        };
    }
    static float rayVsAABB(const glm::vec3& o, const glm::vec3& d,
                       const glm::vec3& bMin, const glm::vec3& bMax)
    {
        float tMin = 0.0f, tMax = 1e30f;
        for (int i = 0; i < 3; ++i) {
            float invD = (std::abs(d[i]) < 1e-8f) ? 1e8f : 1.0f / d[i];
            float t0 = (bMin[i] - o[i]) * invD;
            float t1 = (bMax[i] - o[i]) * invD;
            if (t0 > t1) std::swap(t0, t1);
            tMin = std::max(tMin, t0);
            tMax = std::min(tMax, t1);
            if (tMin > tMax) return -1.0f;
        }
        return tMin;
    }
    static glm::vec3 aabbNormal(const glm::vec3& p,
                            const glm::vec3& bMin, const glm::vec3& bMax)
    {
        glm::vec3 c   = (bMin + bMax) * 0.5f;
        glm::vec3 h   = (bMax - bMin) * 0.5f;
        glm::vec3 d   = p - c;
        glm::vec3 n(0.0f);
        float best = -1.0f;
        for (int i = 0; i < 3; ++i) {
            float v = std::abs(d[i]) / (h[i] < 1e-6f ? 1e-6f : h[i]);
            if (v > best) { best = v; n = glm::vec3(0.0f); n[i] = (d[i] > 0.0f ? 1.0f : -1.0f); }
        }
        return n;
    }
};


#endif //MIMICRY_EXPERIMENTS_RAYCASTSYSTEM_H