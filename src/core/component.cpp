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

