#include "core/component.h"
#include "core/gameobject.h"

void AnimatorComponent::OnEnable(GameObject* owner) {
    //owner->GetComponent<RenderComponent>()->animator = this;
    owner->TraverseChildren([this](GameObject* go) {
        auto* render = go->GetComponent<RenderComponent>();
        if (render) {
            render->animator = this;
            spdlog::info("animator przypisany do: {}", go->name);
        }
        });
}


void ColliderComponent::OnEnable(GameObject* owner) {
    auto renderComponent = owner->GetComponent<RenderComponent>();
    auto transformComponent = owner->GetComponent<TransformComponent>();
    if (renderComponent != nullptr && transformComponent != nullptr)
    {
        this->halfSize = (renderComponent->localObjectAABB.max - renderComponent->localObjectAABB.min) * 0.5f;
        this->halfSize *= transformComponent->scale;
    }
}