#ifndef MIMICRY_EXPERIMENTS_POSTPROCESSING_SYSTEM_H
#define MIMICRY_EXPERIMENTS_POSTPROCESSING_SYSTEM_H
#include "core/system.h"


class PostProcessingSystem  : public System{

public:
    void OnGameObjectUpdated(GameObject* e) override;

    void Update(ECS& ecs, float) override;
};


#endif //MIMICRY_EXPERIMENTS_POSTPROCESSING_SYSTEM_H