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

    //void PhysicsStep(float dt,
    //    TransformComponent& t,
    //    RigidbodyComponent& rb,
    //    ColliderComponent& col,
    //    const std::vector<Entity>& colliders, // inne obiekty
    //    const ComponentManager& cm)
    //{
    //    if (!rb.isStatic) {

    //        // grawitacja
    //        if (rb.useGravity)
    //            rb.velocity.y -= 9.81f * dt;

    //        // integracja ruchu
    //        t.position += rb.velocity * dt;
    //    }

    //    glm::vec3 aPos = t.position + col.offset;
    //    glm::vec3 aHalf = col.halfSize;

    //    for (auto& e : colliders) {
    //        if (e == /*self*/) continue;

    //        auto& t2 = cm.get<TransformComponent>(e);
    //        auto& c2 = cm.get<ColliderComponent>(e);

    //        glm::vec3 bPos = t2.position + c2.offset;
    //        glm::vec3 bHalf = c2.halfSize;

    //        if (AABBIntersect(aPos, aHalf, bPos, bHalf)) {

    //            // prosta korekcja kolizji (push out)
    //            glm::vec3 delta = aPos - bPos;

    //            float overlapX = (aHalf.x + bHalf.x) - std::abs(delta.x);
    //            float overlapY = (aHalf.y + bHalf.y) - std::abs(delta.y);
    //            float overlapZ = (aHalf.z + bHalf.z) - std::abs(delta.z);

    //            if (overlapX < overlapY && overlapX < overlapZ) {
    //                t.position.x += (delta.x > 0 ? overlapX : -overlapX);
    //                rb.velocity.x = 0;
    //            }
    //            else if (overlapY < overlapZ) {
    //                t.position.y += (delta.y > 0 ? overlapY : -overlapY);
    //                rb.velocity.y = 0;
    //            }
    //            else {
    //                t.position.z += (delta.z > 0 ? overlapZ : -overlapZ);
    //                rb.velocity.z = 0;
    //            }
    //        }
    //    }
    //}

    
};

#endif