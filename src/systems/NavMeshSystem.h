//
// Created by kubka on 12.05.2026.
//

#ifndef MIMICRY_EXPERIMENTS_NAVMESHSYSTEM_H
#define MIMICRY_EXPERIMENTS_NAVMESHSYSTEM_H

#include <vector>
#include <glm/glm.hpp>
#include "core/component.h"
#include "fmod.hpp"
#include "core/system.h"

struct NavTriangle {
    glm::vec3 points[3];
    glm::vec3 center;
};

class NavMeshSystem : public System {
    NavMeshSystem() {}
    void Update(ECS& ecs, float deltaTime) override {
    }
    void OnGameObjectUpdated(GameObject* e) override {
    }
    void Bake(ECS& ecs);
    void DebugDraw();
private:
    std::vector<NavTriangle> m_triangles;
    void Triangulate(const std::vector<glm::vec2>& vertices, const std::vector<std::pair<int, int>>& edges);
};


#endif //MIMICRY_EXPERIMENTS_NAVMESHSYSTEM_H
