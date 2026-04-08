#include "transform_system.h"

void TransformSystem::updateTransforms(GameObject* obj) {
    auto* tComp = obj->GetComponent<TransformComponent>();
    if (!tComp) return;

    if (tComp->isDirty) {
        forceUpdateSelfAndChild(obj);
    }
    else {
        for (auto* child : obj->children) {
            updateTransforms(child);
        }
    }
}

void TransformSystem::forceUpdateSelfAndChild(GameObject* obj) {
    auto* tComp = obj->GetComponent<TransformComponent>();
    if (!tComp) return;

    if (obj->parent) {
        auto* parentT = obj->parent->GetComponent<TransformComponent>();
        if (parentT)
            helper.computeModelMatrix(parentT->modelMatrix, *tComp);
        else
            helper.computeModelMatrix(*tComp);
    }
    else {
        helper.computeModelMatrix(*tComp);
    }

    for (auto* child : obj->children) {
        forceUpdateSelfAndChild(child);
    }
}