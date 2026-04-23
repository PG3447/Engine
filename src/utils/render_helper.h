#ifndef RENDER_HELPER_H
#define RENDER_HELPER_H

#include "../core/gameobject.h"
#include "../core/component.h"
#include "../material.h"
#include <memory>

class RenderHelper {
public:
    static void SetMaterial(GameObject* root, std::shared_ptr<Material> newMaterial, bool includeChildren = true) {
        if (!root) return;

        if (auto* renderComp = root->GetComponent<RenderComponent>()) {
            renderComp->materialOverride = newMaterial;
        }

        if (includeChildren) {
            auto childRenderers = root->GetComponentsInChildren<RenderComponent>();
            for (auto* renderComp : childRenderers) {
                renderComp->materialOverride = newMaterial;
            }
        }
    }
};

#endif