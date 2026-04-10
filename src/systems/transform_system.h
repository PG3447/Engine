#ifndef TRANSFORM_SYSTEM_H
#define TRANSFORM_SYSTEM_H

#include "core/ecs.h"
#include "utils/transform_helper.h"

class TransformSystem : public System {
private:

    
    void forceUpdateSelfAndChild(GameObject* obj);
public:
    TransformSystem(ECS& ecs) {};

    void OnGameObjectUpdated(GameObject* e) override {};

    void updateSelfAndChild(GameObject* obj);
    void Update(ECS& ecs) override {};

};

#endif