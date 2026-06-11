#include "physics_system.h"
#include "transform.h"
#include <spdlog/spdlog.h>


PhysicsSystem::PhysicsSystem(ECS& ecs) {
    query = ecs.CreateQuery<TransformComponent, RigidbodyComponent, ColliderComponent>();
    colliderOnlyQuery = ecs.CreateQuery<TransformComponent, ColliderComponent>();
}

void PhysicsSystem::OnGameObjectUpdated(GameObject* e) {
    query->OnGameObjectUpdated(e);
    colliderOnlyQuery->OnGameObjectUpdated(e);
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

                float e = std::min(rbA->bounce, rbB->bounce);

                // względna prędkość wzdłuż osi kolizji
                float relVelY = rbA->velocity.y - rbB->velocity.y;

                // impuls
                float impulse = -(1.0f + e) * relVelY;

                float massA = rbA->isStatic ? 0.0f : 1.0f / rbA->mass;
                float massB = rbB->isStatic ? 0.0f : 1.0f / rbB->mass;
                float totalInvMass = massA + massB;

                if (totalInvMass > 0.0f)
                    impulse /= totalInvMass;

                if (!rbA->isStatic)
                {
                    rbA->physicsPosition.x += overlapX * dir;
                    rbA->velocity.x += impulse * massA;
                    rbA->physicsPosition.x = rbA->physicsPosition.x;
                    //tA->isDirty = true;
                }

                if (!rbB->isStatic)
                {
                    rbB->physicsPosition.x -= overlapX * dir;
                    rbB->velocity.x -= impulse * massB;
                    rbB->physicsPosition.x = rbB->physicsPosition.x;
                    //tB->isDirty = true;
                }
                
            }
            else if (overlapY < overlapZ) {

                float dir = (delta.y > 0 ? 1.0f : -1.0f);

                float e = std::min(rbA->bounce, rbB->bounce);

                // względna prędkość wzdłuż osi kolizji
                float relVelY = rbA->velocity.y - rbB->velocity.y;

                // impuls
                float impulse = -(1.0f + e) * relVelY;

                float massA = rbA->isStatic ? 0.0f : 1.0f / rbA->mass;
                float massB = rbB->isStatic ? 0.0f : 1.0f / rbB->mass;
                float totalInvMass = massA + massB;

                if (totalInvMass > 0.0f)
                    impulse /= totalInvMass;

                if (!rbA->isStatic)
                {
                    rbA->physicsPosition.y += overlapY * dir;
                    rbA->velocity.y += impulse * massA;
                    rbA->physicsPosition.y = rbA->physicsPosition.y;
                    //tA->isDirty = true;
                }

                if (!rbB->isStatic)
                {
                    rbB->physicsPosition.y -= overlapY * dir;
                    rbB->velocity.y -= impulse * massB;
                    rbB->physicsPosition.y = rbB->physicsPosition.y;
                    //tB->isDirty = true;
                }

            }
            else {

                float dir = (delta.z > 0 ? 1.0f : -1.0f);

                float e = std::min(rbA->bounce, rbB->bounce);

                // względna prędkość wzdłuż osi kolizji
                float relVelY = rbA->velocity.y - rbB->velocity.y;

                // impuls
                float impulse = -(1.0f + e) * relVelY;

                float massA = rbA->isStatic ? 0.0f : 1.0f / rbA->mass;
                float massB = rbB->isStatic ? 0.0f : 1.0f / rbB->mass;
                float totalInvMass = massA + massB;

                if (totalInvMass > 0.0f)
                    impulse /= totalInvMass;

                if (!rbA->isStatic)
                {
                    rbA->physicsPosition.z += overlapZ * dir;
                    rbA->velocity.z += impulse * massA;
                    rbA->physicsPosition.z = rbA->physicsPosition.z;
                    //tA->isDirty = true;
                }

                if (!rbB->isStatic)
                {
                    rbB->physicsPosition.z -= overlapZ * dir;
                    rbB->velocity.z -= impulse * massB;
                    rbB->physicsPosition.z = rbB->physicsPosition.z;
                    //tB->isDirty = true;
                }

            }
               
        }
    }

    auto& coTransforms = std::get<0>(colliderOnlyQuery->componentsVectors);
    auto& coColliders = std::get<1>(colliderOnlyQuery->componentsVectors);

    for (size_t i = 0; i < query->gameobjects.size(); i++) {
        auto* rb = rigidbodies[i];
        auto* cA = colliders[i];

        for (size_t j = 0; j < colliderOnlyQuery->gameobjects.size(); j++) {
            auto* tB = coTransforms[j];
            auto* cB = coColliders[j];

            // pomiń jeśli ten sam obiekt jest w obu query
            if (query->gameobjects[i] == colliderOnlyQuery->gameobjects[j])
                continue;

            glm::vec3 posA = rb->physicsPosition + cA->offset;
            glm::vec3 posB = tB->position + cB->offset;

            if (!AABB(posA, cA->halfSize, posB, cB->halfSize))
                continue;

            glm::vec3 delta = posA - posB;

            float overlapX = (cA->halfSize.x + cB->halfSize.x) - std::abs(delta.x);
            float overlapY = (cA->halfSize.y + cB->halfSize.y) - std::abs(delta.y);
            float overlapZ = (cA->halfSize.z + cB->halfSize.z) - std::abs(delta.z);

            if (overlapX < overlapY && overlapX < overlapZ) {
                float dir = (delta.x > 0 ? 1.0f : -1.0f);
                rb->physicsPosition.x += overlapX * dir;
                rb->previousPosition.x = rb->physicsPosition.x;
                rb->velocity.x = 0;
                rb->acceleration.x = 0;
            }
            else if (overlapY < overlapZ) {
                float dir = (delta.y > 0 ? 1.0f : -1.0f);
                rb->physicsPosition.y += overlapY * dir;
                rb->previousPosition.y = rb->physicsPosition.y;
                rb->velocity.y = 0;
                rb->acceleration.y = 0;
            }
            else {
                float dir = (delta.z > 0 ? 1.0f : -1.0f);
                rb->physicsPosition.z += overlapZ * dir;
                rb->previousPosition.z = rb->physicsPosition.z;
                rb->velocity.z = 0;
                rb->acceleration.z = 0;
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