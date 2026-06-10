#ifndef PHYSICS_SYSTEM_H
#define PHYSICS_SYSTEM_H

#include "core/ecs.h"

struct CollisionPoints {
    glm::vec3 A; // Furthest point of A into B
    glm::vec3 B; // Furthest point of B into A
    glm::vec3 Normal; // B - A normalized
    float Depth;      // Length of B -A
    bool HasCollision;
};



class PhysicsSystem : public System {
private:
    //std::vector<CollisionObject*> m_objects;

    Query<TransformComponent, RigidbodyComponent, ColliderComponent>* query;

public:
    PhysicsSystem(ECS& ecs);

    void OnGameObjectUpdated(GameObject* e) override;

    void Update(ECS&, float dt) override;

    void ApplyForce(GameObject* e, float fx, float fy);


    bool AABB(
        const glm::vec3& aPos, const glm::vec3& aHalf,
        const glm::vec3& bPos, const glm::vec3& bHalf)
    {
        return std::abs(aPos.x - bPos.x) <= (aHalf.x + bHalf.x) &&
            std::abs(aPos.y - bPos.y) <= (aHalf.y + bHalf.y) &&
            std::abs(aPos.z - bPos.z) <= (aHalf.z + bHalf.z);
    }
};

#endif
