#include "transform_system.h"
#include <spdlog/spdlog.h>

void TransformSystem::updateSelfAndChild(GameObject* obj) {
    auto* tComp = obj->GetComponent<TransformComponent>();
    if (!tComp) return;

    if (tComp->isDirty)
    {
        forceUpdateSelfAndChild(obj);
        return;
    }

    for (auto* child : obj->GetChildren()) {
        updateSelfAndChild(child);
    }
    
}


void TransformSystem::forceUpdateSelfAndChild(GameObject* obj)
{
    auto* tComp = obj->GetComponent<TransformComponent>();
    if (!tComp) return;

    if (obj->GetParent())
    {
        auto* parentT = obj->GetParent()->GetComponent<TransformComponent>();

        if (parentT)
            TransformHelper::computeModelMatrix(parentT->modelMatrix, *tComp);
        else
            TransformHelper::computeModelMatrix(*tComp);
    }
    else
    {
        TransformHelper::computeModelMatrix(*tComp);
    }

    for (auto* child : obj->GetChildren())
    {
        forceUpdateSelfAndChild(child);
    }
}