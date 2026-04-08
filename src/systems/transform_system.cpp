#include "transform_system.h"

void TransformSystem::updateSelfAndChild(GameObject* obj) {
    auto* tComp = obj->GetComponent<TransformComponent>();
    if (!tComp) return;

    if (tComp->isDirty)
    {
        forceUpdateSelfAndChild(obj);
    }
    else
    {
        for (auto* child : obj->children) {
            updateSelfAndChild(child);
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



void updateSelfAndChild()
{
    if (transform.isDirty()) {
        forceUpdateSelfAndChild();
        return;
    }

    for (auto&& child : children)
    {
        child->updateSelfAndChild();
    }
}

//Force update of transform even if local space don't change
void forceUpdateSelfAndChild()
{
    if (parent)
        transform.computeModelMatrix(parent->transform.getModelMatrix());
    else
        transform.computeModelMatrix();

    for (auto&& child : children)
    {
        child->forceUpdateSelfAndChild();
    }
}