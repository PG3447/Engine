#include "physics_system.h"


PhysicsSystem::PhysicsSystem(ECS& ecs) {
    query = ecs.CreateQuery<TransformComponent, RigidbodyComponent>();
}

void PhysicsSystem::OnGameObjectUpdated(GameObject* e) {
    query->OnGameObjectUpdated(e); // forward do query
}

void PhysicsSystem::Update(ECS&) {
    auto& transforms = std::get<0>(query->componentsVectors);
    auto& rigidbodies = std::get<1>(query->componentsVectors);


    for (size_t i = 0; i < query->gameobjects.size(); i++) {
        float ay = -9.8f; // grawitacja

        rigidbodies[i]->vy += ay; // aktualizacja prêdkoœci
        transforms[i]->position.y += rigidbodies[i]->vy; // aktualizacja pozycji

        transforms[i]->isDirty = true;

        //if (transforms[i]->isDirty) {
        //    Transform::computeModelMatrix(*transforms[i]);
        //}
    }
}

void PhysicsSystem::ApplyForce(GameObject* e, float fx, float fy)  {
    auto* rb = e->GetComponent<RigidbodyComponent>();
    if (rb) {
        rb->vx += fx;
        rb->vy += fy;
    }
}