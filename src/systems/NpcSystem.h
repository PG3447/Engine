#pragma once
#include "core/component.h"
#include "NavMeshSystem.h"
#include <core/ecs.h>
#include <core/gameobject.h>
#include <systems/NavMeshSystem.h>

class NpcSystem : public System {
public:
    explicit NpcSystem(ECS& ecs);

    void OnGameObjectUpdated(GameObject* go) override;
    void Update(ECS& ecs, float dt) override;

private:
    Query<TransformComponent, NavPathComponent, CockroachLeaderComponent>*   leaderQuery_   = nullptr;
    Query<TransformComponent, NavPathComponent, CockroachFollowerComponent>* followerQuery_ = nullptr;

    NavMeshComponent* cachedNavMesh_ = nullptr;

    void UpdateLeader(GameObject* go,
                      TransformComponent& tr,
                      NavPathComponent&   nav,
                      CockroachLeaderComponent& leader,
                      const std::vector<glm::vec3>& playerPositions,
                      float dt);

    bool PlayersInRange(const glm::vec3& pos, float radius,
                        const std::vector<glm::vec3>& playerPositions) const;

    void UpdateFollower(GameObject* go,
                        TransformComponent& tr,
                        NavPathComponent&   nav,
                        CockroachFollowerComponent& follower,
                        float dt);


    void SendTo(NavPathComponent& nav, const glm::vec3& goal);

    glm::vec3 RandomPointAround(const glm::vec3& center, float radius) const;
    glm::vec3 RandomPointOnNavMesh(const NavMeshData& data) const;
    glm::vec3 ClampToNavMesh(const glm::vec3& pos) const;

    bool HasArrived(const NavPathComponent& nav) const;
};
