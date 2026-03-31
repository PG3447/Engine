#ifndef PHYSICS_SYSTEM_H
#define PHYSICS_SYSTEM_H

#include "transform.h"
#include "core/ecs.h"


class PhysicsSystem : public System {
private:
    Query<TransformComponent, RigidbodyComponent>* query;

public:
    PhysicsSystem(ECS& ecs);

    void OnGameObjectUpdated(GameObject* e) override;

    void Update(ECS&) override;

    void ApplyForce(GameObject* e, float fx, float fy);

};

#endif