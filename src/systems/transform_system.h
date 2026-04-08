#ifndef TRANSFORM_SYSTEM_H
#define TRANSFORM_SYSTEM_H

#include "core/ecs.h"
#include "transform.h"

class TransformSystem : public System {
private:
    Transform helper;

    
    void forceUpdateSelfAndChild(GameObject* obj);
public:

    void updateSelfAndChild(GameObject* obj);
    void Update(ECS& ecs);
};

#endif