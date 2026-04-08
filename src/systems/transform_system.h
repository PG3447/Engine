#ifndef TRANSFORM_SYSTEM_H
#define TRANSFORM_SYSTEM_H

#include "core/ecs.h"
#include "transform.h"

class TransformSystem : public System {
private:
    Transform helper;

    void updateTransforms(GameObject* obj);
    void forceUpdateSelfAndChild(GameObject* obj);

    glm::mat4 computeModelMatrix(const TransformComponent& t, const glm::mat4& parentMatrix = glm::mat4(1.0f));
public:

    void Update(ECS& ecs);
};

#endif