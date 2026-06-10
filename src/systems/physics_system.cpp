#include "physics_system.h"
#include "transform.h"
#include <spdlog/spdlog.h>


PhysicsSystem::PhysicsSystem(ECS& ecs) {
    query = ecs.CreateQuery<TransformComponent, RigidbodyComponent, ColliderComponent>();
}

void PhysicsSystem::OnGameObjectUpdated(GameObject* e) {
    query->OnGameObjectUpdated(e);
}

void PhysicsSystem::Init() {
    auto& transforms = std::get<0>(query->componentsVectors);
    auto& rigidbodies = std::get<1>(query->componentsVectors);

    for (size_t i = 0; i < query->gameobjects.size(); i++) {
        auto* tr = transforms[i];
        auto* rb = rigidbodies[i];
        rb->physicsPosition = tr->position;
        rb->previousPosition = tr->position;
    }
}

void PhysicsSystem::Update(ECS&, float dt) {

    physicsAccumulator += dt;

    while (physicsAccumulator >= fixedDeltaTime)
    {
        FixedUpdate(fixedDeltaTime);

        physicsAccumulator -= fixedDeltaTime;
    }

    float alpha = physicsAccumulator / fixedDeltaTime;
    Interpolate(alpha);
}

void PhysicsSystem::Interpolate(float alpha)
{
    auto& transforms = std::get<0>(query->componentsVectors);
    auto& rigidbodies = std::get<1>(query->componentsVectors);

    for (size_t i = 0; i < query->gameobjects.size(); i++) {
        auto* tr = transforms[i];
        auto* rb = rigidbodies[i];
        if (rb->isStatic) continue;

        tr->position = glm::mix(rb->previousPosition, rb->physicsPosition, alpha);
        tr->isDirty = true;
    }

}

void PhysicsSystem::FixedUpdate(float fixedDeltaTime)
{
    auto& transforms = std::get<0>(query->componentsVectors);
    auto& rigidbodies = std::get<1>(query->componentsVectors);
    auto& colliders = std::get<2>(query->componentsVectors);

    //dt = 1.0f / 60.0f;

    // RUCH
    for (size_t i = 0; i < query->gameobjects.size(); i++) {

        auto* tr = transforms[i];
        auto* rb = rigidbodies[i];

        rb->previousPosition = rb->physicsPosition;

        if (tr->isDirty)
        {
            rb->physicsPosition = tr->position;
            //rb->previousPosition = tr->position;
        }

        if (rb->isStatic)
            continue;

        // reset acceleration
        glm::vec3 acc = rb->acceleration;

        // grawitacja
        if (rb->useGravity)
            acc.y += -9.81f;

        rb->velocity += acc * fixedDeltaTime;
        //rb->velocity *= rb->damping;

        rb->physicsPosition += rb->velocity * fixedDeltaTime;

        //tr->isDirty = true;
        //rb->physicsPosition = tr->position;
    }

    // KOLIZJE (AABB)
    for (size_t i = 0; i < query->gameobjects.size(); i++) {
        for (size_t j = i + 1; j < query->gameobjects.size(); j++) {

            //auto* tA = transforms[i];
            //auto* tB = transforms[j];

            auto* cA = colliders[i];
            auto* cB = colliders[j];

            auto* rbA = rigidbodies[i];
            auto* rbB = rigidbodies[j];

            glm::vec3 posA = rbA->physicsPosition + cA->offset;
            glm::vec3 posB = rbB->physicsPosition + cB->offset;

            glm::vec3 halfA = cA->halfSize;
            glm::vec3 halfB = cB->halfSize;

            if (!AABB(posA, halfA, posB, halfB))
                continue;

            //kierunek
            glm::vec3 delta = posA - posB;

            //obliczenie jak bardzo sie przenikaja
            float overlapX = (halfA.x + halfB.x) - std::abs(delta.x);
            float overlapY = (halfA.y + halfB.y) - std::abs(delta.y);
            float overlapZ = (halfA.z + halfB.z) - std::abs(delta.z);

            // wybór osi dla ktorej wypchniecie jest najkrotsze
            if (overlapX < overlapY && overlapX < overlapZ) {

                float dir = (delta.x > 0 ? 1.0f : -1.0f);

                if (!rbA->isStatic)
                {
                    rbA->physicsPosition.x += overlapX * dir;
                    rbA->velocity.x = 0;
                    rbA->physicsPosition.x = rbA->physicsPosition.x;
                    //tA->isDirty = true;
                }

                if (!rbB->isStatic)
                {
                    rbB->physicsPosition.x -= overlapX * dir;
                    rbB->velocity.x = 0;
                    rbB->physicsPosition.x = rbB->physicsPosition.x;
                    //tB->isDirty = true;
                }
                
            }
            else if (overlapY < overlapZ) {

                float dir = (delta.y > 0 ? 1.0f : -1.0f);

                if (!rbA->isStatic)
                {
                    rbA->physicsPosition.y += overlapY * dir;
                    rbA->velocity.y = 0;
                    rbA->physicsPosition.y = rbA->physicsPosition.y;
                    //tA->isDirty = true;
                }

                if (!rbB->isStatic)
                {
                    rbB->physicsPosition.y -= overlapY * dir;
                    rbB->velocity.y = 0;
                    rbB->physicsPosition.y = rbB->physicsPosition.y;
                    //tB->isDirty = true;
                }

            }
            else {

                float dir = (delta.z > 0 ? 1.0f : -1.0f);

                if (!rbA->isStatic)
                {
                    rbA->physicsPosition.z += overlapZ * dir;
                    rbA->velocity.z = 0;
                    rbA->physicsPosition.z = rbA->physicsPosition.z;
                    //tA->isDirty = true;
                }

                if (!rbB->isStatic)
                {
                    rbB->physicsPosition.z -= overlapZ * dir;
                    rbB->velocity.z = 0;
                    rbB->physicsPosition.z = rbB->physicsPosition.z;
                    //tB->isDirty = true;
                }

            }
               
        }
    }
}

void PhysicsSystem::ApplyForce(GameObject* e, float fx, float fy)  {
    auto* rb = e->GetComponent<RigidbodyComponent>();
    if (rb) {
        rb->velocity.x += fx;
        rb->velocity.y += fy;
    }
}