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

    spdlog::info("skala");
    spdlog::info(tComp->scale.x);
    spdlog::info(tComp->position.z);
    spdlog::info("transform dziala update");

    for (auto* child : obj->GetChildren())
    {
        forceUpdateSelfAndChild(child);
    }
}

/*


            // Przelicz modelMatrix jeśli jest dirty
            if (t->isDirty) {
                Transform::computeModelMatrix(*t); // helper statyczny lub instancja
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
}*/