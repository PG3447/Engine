#ifndef TRANSFORM_SYSTEM_H
#define TRANSFORM_SYSTEM_H

#include "core/ecs.h"
#include "transform.h"

class TransformSystem : public System {
private:
    Transform helper;

    void updateSelfAndChild(GameObject* obj);
    void forceUpdateSelfAndChild(GameObject* obj);
public:

    void Update(ECS& ecs);
};

#endif