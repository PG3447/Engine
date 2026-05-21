#include "physics_system.h"
#include "transform.h"
#include <spdlog/spdlog.h>


PhysicsSystem::PhysicsSystem(ECS& ecs) {
    query = ecs.CreateQuery<TransformComponent, RigidbodyComponent, ColliderComponent>();
}

void PhysicsSystem::OnGameObjectUpdated(GameObject* e) {
    query->OnGameObjectUpdated(e);
}

void PhysicsSystem::Update(ECS&, float dt) {

    auto& transforms = std::get<0>(query->componentsVectors);
    auto& rigidbodies = std::get<1>(query->componentsVectors);
    auto& colliders = std::get<2>(query->componentsVectors);

    dt = 1.0f / 60.0f;

    // RUCH
    for (size_t i = 0; i < query->gameobjects.size(); i++) {

        auto* tr = transforms[i];
        auto* rb = rigidbodies[i];

        if (rb->isStatic)
            continue;

        // reset acceleration
        glm::vec3 acc = rb->acceleration;

        // grawitacja
        if (rb->useGravity)
            acc.y += -9.81f;

        rb->velocity += acc * dt;
        //rb->velocity *= rb->damping;

        tr->position += rb->velocity * dt;

        tr->isDirty = true;
    }

    // KOLIZJE (AABB)
    for (size_t i = 0; i < query->gameobjects.size(); i++) {
        for (size_t j = i + 1; j < query->gameobjects.size(); j++) {

            auto* tA = transforms[i];
            auto* tB = transforms[j];

            auto* cA = colliders[i];
            auto* cB = colliders[j];

            auto* rbA = rigidbodies[i];
            auto* rbB = rigidbodies[j];

            glm::vec3 posA = tA->position + cA->offset;
            glm::vec3 posB = tB->position + cB->offset;

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
                    tA->position.x += overlapX * dir;
                if (!rbB->isStatic)
                    tB->position.x -= overlapX * dir;

                rbA->velocity.x = 0;
                rbB->velocity.x = 0;
            }
            else if (overlapY < overlapZ) {

                float dir = (delta.y > 0 ? 1.0f : -1.0f);

                if (!rbA->isStatic)
                    tA->position.y += overlapY * dir;
                if (!rbB->isStatic)
                    tB->position.y -= overlapY * dir;

                rbA->velocity.y = 0;
                rbB->velocity.y = 0;
            }
            else {

                float dir = (delta.z > 0 ? 1.0f : -1.0f);

                if (!rbA->isStatic)
                    tA->position.z += overlapZ * dir;
                if (!rbB->isStatic)
                    tB->position.z -= overlapZ * dir;

                rbA->velocity.z = 0;
                rbB->velocity.z = 0;
            }

            tA->isDirty = true;
            tB->isDirty = true;
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
