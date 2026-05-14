//
// Created by CKLG on 14.05.2026.
//

#ifndef MIMICRY_EXPERIMENTS_NAVPATHSYSTEM_H
#define MIMICRY_EXPERIMENTS_NAVPATHSYSTEM_H


#include "core/ecs.h"
#include "core/component.h"
#include <vector>
#include <queue>
#include <unordered_map>

class NavPathSystem : public System {
public:
    explicit NavPathSystem(ECS& ecs);

    void OnGameObjectUpdated(GameObject* e) override;
    void Update(ECS& ecs, float dt) override;

    // Oblicz sciezke A* od start do goal, zapisz w comp
    bool RequestPath(NavPathComponent& comp,
                     const glm::vec3& start,
                     const glm::vec3& goal,
                     const NavMeshData& navData);

    // Wylosuj losowy punkt na navmeshu
    glm::vec3 RandomPointOnNavMesh(const NavMeshData& navData);

private:
    struct AStarNode {
        int triIndex;
        float g;    // Koszt od startu
        float f;    // g + h

        bool operator>(const AStarNode& o) const { return f > o.f; }
    };

    // Zwraca liste indeksow trojkatow (od start do goal)
    std::vector<int> AStar(
        int startTri,
        int goalTri,
        const NavMeshData& navData);

    float Heuristic(int triA, int triB, const NavMeshData& navData) const;

    // Portal = krawedz wspolna dwoch sasiadujacych trojkatow
    struct Portal {
        glm::vec3 left;
        glm::vec3 right;
    };

    std::vector<glm::vec3> FunnelPath(
        const std::vector<int>& triPath,
        const glm::vec3& startPos,
        const glm::vec3& goalPos,
        const NavMeshData& navData);

    // Wyciagnij portal (wspolna krawedz) miedzy dwoma sasiadami
    bool GetPortal(
        int fromTri,
        int toTri,
        const NavMeshData& navData,
        glm::vec3& outLeft,
        glm::vec3& outRight) const;

    // Pomocnicze: cross product 2D (XZ)
    float Cross2D(const glm::vec3& o,
                  const glm::vec3& a,
                  const glm::vec3& b) const;

    //  Ruch agenta
    void MoveAgent(GameObject* go,
                   NavPathComponent& comp,
                   float dt);

    //  Query
    Query<TransformComponent, NavPathComponent>* agentQuery_ = nullptr;

    // Cache do NavMesh - pobieramy raz w Update
    NavMeshComponent* cachedNavMesh_ = nullptr;
};


#endif //MIMICRY_EXPERIMENTS_NAVPATHSYSTEM_H