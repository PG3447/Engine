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

    void Update(ECS&) override;

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

//namespace algo {
//
//    CollisionPoints FindSphereSphereCollisionPoints(
//        const SphereCollider* a, const Transform* ta,
//        const SphereCollider* b, const Transform* tb);
//
//    CollisionPoints FindSpherePlaneCollisionPoints(
//        const SphereCollider* a, const Transform* ta,
//        const PlaneCollider* b, const Transform* tb);
//
//    CollisionPoints FindPlaneSphereCollisionPoints(
//        const PlaneCollider* a, const Transform* ta,
//        const SphereCollider* b, const Transform* tb);
//
//}
//
//
//struct Collider {
//    virtual CollisionPoints TestCollision(
//        const Transform* transform,
//        const Collider* collider,
//        const Transform* colliderTransform) const = 0;
//    virtual CollisionPoints TestCollision(
//        const Transform* transform,
//        const SphereCollider* sphere,
//        const Transform* sphereTransform) const = 0;
//    virtual CollisionPoints TestCollision(
//        const Transform* transform,
//        const PlaneCollider* plane,
//        const Transform* planeTransform) const = 0;
//};
//
//
//struct SphereCollider : Collider {
//    glm::vec3 Center;
//    float Radius;
//
//    CollisionPoints TestCollision(
//        const Transform* transform,
//        const Collider* collider,
//        const Transform* colliderTransform) const override
//    {
//        return collider->TestCollision(colliderTransform, this, transform);
//    }
//
//    CollisionPoints TestCollision(
//        const Transform* transform,
//        const SphereCollider* sphere,
//        const Transform* sphereTransform) const override
//    {
//        return algo::FindSphereSphereCollisionPoints(this, transform, sphere, sphereTransform);
//    }
//
//    CollisionPoints TestCollision(
//        const Transform* transform,
//        const PlaneCollider* plane,
//        const Transform* planeTransform) const override
//    {
//        return algo::FindSpherePlaneCollisionPoints(this, transform, plane, planeTransform);
//    }
//};
//
//
//struct PlaneCollider : Collider
//{
//    glm::vec3 Plane;
//    float Distance;
//
//    CollisionPoints TestCollision(
//        const Transform* transform,
//        const Collider* collider,
//        const Transform* colliderTransform) const override
//    {
//        return collider->TestCollision(colliderTransform, this, transform);
//    }
//
//    CollisionPoints TestCollision(
//        const Transform* transform,
//        const SphereCollider* sphere,
//        const Transform* sphereTransform) const override
//    {
//        return algo::FindPlaneSphereCollisionPoints(this, transform, sphere, sphereTransform);
//    }
//
//    CollisionPoints TestCollision(
//        const Transform* transform,
//        const PlaneCollider* plane,
//        const Transform* planeTransform) const override
//    {
//        return {};
//    }
//};
//
//
//struct Rigidbody : CollisionObject {
//    float mass;
//    glm::vec3 velocity;
//    glm::vec3 force;
//
//    glm::vec3 gravity;
//    bool takersGravity;
//};
//
//struct CollisionObject {
//    Collider* collider;
//    Transform* transform;
//
//    bool IsDynamic;
//    //bool IsTrigger;
//
//    std::function<void(Collision&, float)> OnCollision;
//};
//
//struct Collision {
//    CollisionObject* ObjA;
//    CollisionObject* ObjB;
//    CollisionPoints points;
//};
//


//void Step(float dt)
//{
//    // 1. Apply forces → velocity
//    for (auto* obj : m_objects) {
//        if (!obj->IsDynamic) continue;

//        auto* rb = static_cast<Rigidbody*>(obj);

//        if (rb->takersGravity) {
//            rb->force += rb->mass * rb->gravity;
//        }

//        rb->velocity += (rb->force / std::max(rb->mass, 0.0001f)) * dt;
//    }

//    // 2. Integrate POSITION (ważne: przed kolizją)
//    for (auto* obj : m_objects) {
//        if (!obj->IsDynamic) continue;

//        auto* rb = static_cast<Rigidbody*>(obj);
//        obj->transform->position += rb->velocity * dt;
//    }

//    // 3. Solve collisions (ITERACYJNIE)
//    for (int i = 0; i < 5; i++) {
//        ResolveCollisions(dt);
//    }

//    // 4. Clear forces
//    for (auto* obj : m_objects) {
//        if (!obj->IsDynamic) continue;

//        auto* rb = static_cast<Rigidbody*>(obj);
//        rb->force = glm::vec3(0.0f);
//    }
//}


//void ResolveCollisions(float dt)
//{
//    for (size_t i = 0; i < m_objects.size(); ++i) {
//        for (size_t j = i + 1; j < m_objects.size(); ++j) {

//            auto* a = m_objects[i];
//            auto* b = m_objects[j];

//            if (!a->collider || !b->collider) continue;

//            CollisionPoints points =
//                a->collider->TestCollision(a->transform, b->collider, b->transform);

//            if (!points.HasCollision) continue;

//            if (!a->IsDynamic && !b->IsDynamic) continue;

//            auto* rbA = a->IsDynamic ? static_cast<Rigidbody*>(a) : nullptr;
//            auto* rbB = b->IsDynamic ? static_cast<Rigidbody*>(b) : nullptr;

//            float invMassA = rbA ? 1.0f / std::max(rbA->mass, 0.0001f) : 0.0f;
//            float invMassB = rbB ? 1.0f / std::max(rbB->mass, 0.0001f) : 0.0f;

//            float invMassSum = invMassA + invMassB;
//            if (invMassSum == 0.0f) continue;

//            // -------------------------
//            // 1. POSITION CORRECTION (stabilized)
//            // -------------------------

//            float slop = 0.01f;
//            float percent = 0.8f;

//            float correctionMag =
//                std::max(points.Depth - slop, 0.0f) / invMassSum * percent;

//            glm::vec3 correction = correctionMag * points.Normal;

//            if (rbA)
//                a->transform->position -= correction * invMassA;

//            if (rbB)
//                b->transform->position += correction * invMassB;

//            // -------------------------
//            // 2. IMPULSE
//            // -------------------------

//            glm::vec3 velA = rbA ? rbA->velocity : glm::vec3(0.0f);
//            glm::vec3 velB = rbB ? rbB->velocity : glm::vec3(0.0f);

//            glm::vec3 relativeVel = velB - velA;
//            float velAlongNormal = glm::dot(relativeVel, points.Normal);

//            if (velAlongNormal > 0.0f) continue;

//            float restitution = 0.5f;

//            float j = -(1.0f + restitution) * velAlongNormal;
//            j /= invMassSum;

//            glm::vec3 impulse = j * points.Normal;

//            if (rbA)
//                rbA->velocity -= impulse * invMassA;

//            if (rbB)
//                rbB->velocity += impulse * invMassB;

//            // -------------------------
//            // 3. CALLBACKS
//            // -------------------------
//            if (a->OnCollision) {
//                Collision c{ *a, *b, points };
//                a->OnCollision(c, dt);
//            }

//            if (b->OnCollision) {
//                Collision c{ *a, *b, points };
//                b->OnCollision(c, dt);
//            }
//        }
//    }
//}



    //void Step(float dt)
    //{
    //    for (Object ob : m_object) {
    //        obj->Force += obj->Mass * gravity;

    //        obj->velocity += force / mass * dt;
    //        obj->transform->position += velocity * dt;
    //        
    //        obj->Force = glm::vec3 { 0.0f, 0.0f, 0.0f };
    //    }
    //}

    //void ResolveCollisions(float dt)
    //{
    //    std::vector<Collision> collisions;
    //    for (Object* a : m_objects) {
    //        for (Object* b : m_objects) {
    //            if (a == b) break; // unique pairs
    //            if (!a->Collider || !b->Collider) continue; // both have colliders
    //            CollisionPoints points = a->Collider->TestCollision(a->Transform, b->Collider, b->Transform);
    //            if (points.HasCollision) {
    //                collisions.emplace_back(a, b, points);
    //            }
    //        }
    //    }

    //    // Solve collisions
    //}