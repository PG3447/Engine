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

        if (auto* renderComp = root->GetComponent<RenderComponent>())
        {
            for (auto& mesh : renderComp->meshes)
            {
                mesh.material = newMaterial;
            }
        }

        if (includeChildren)
        {
            auto childRenderers = root->GetComponentsInChildren<RenderComponent>();

            for (auto* renderComp : childRenderers)
            {
                if (!renderComp) continue;

                for (auto& mesh : renderComp->meshes)
                {
                    mesh.material = newMaterial;
                }
            }
        }
    }

    static void SetDiffuseColor(GameObject* root, const glm::vec3& color, bool includeChildren = true)
    {
        if (!root) return;

        auto apply = [&](RenderComponent* renderComp)
            {
                for (auto& mesh : renderComp->meshes)
                {
                    if (mesh.material)
                    {
                        mesh.material->diffuseColor = color;
                    }
                }
            };

        if (auto* renderComp = root->GetComponent<RenderComponent>())
            apply(renderComp);

        if (includeChildren)
        {
            auto childRenderers = root->GetComponentsInChildren<RenderComponent>();

            for (auto* renderComp : childRenderers)
            {
                if (!renderComp) continue;
                apply(renderComp);
            }
        }
    }

    static void SetShininess(GameObject* root, float value, bool includeChildren = true)
    {
        if (!root) return;

        auto apply = [&](RenderComponent* renderComp)
            {
                for (auto& mesh : renderComp->meshes)
                {
                    if (mesh.material)
                        mesh.material->shininess = value;
                }
            };

        if (auto* renderComp = root->GetComponent<RenderComponent>())
            apply(renderComp);

        if (includeChildren)
        {
            auto childRenderers = root->GetComponentsInChildren<RenderComponent>();

            for (auto* renderComp : childRenderers)
            {
                if (!renderComp) continue;
                apply(renderComp);
            }
        }
    }

    static void SetSpecularTexture(GameObject* root, GLuint specularTex, bool includeChildren = true)
    {
        if (!root) return;

        auto apply = [&](RenderComponent* renderComp)
            {
                for (auto& mesh : renderComp->meshes)
                {
                    if (mesh.material)
                    {
                        mesh.material->specularMap = specularTex;
                    }
                }
            };

        if (auto* renderComp = root->GetComponent<RenderComponent>())
            apply(renderComp);

        if (includeChildren)
        {
            auto childRenderers = root->GetComponentsInChildren<RenderComponent>();

            for (auto* renderComp : childRenderers)
            {
                if (!renderComp) continue;
                apply(renderComp);
            }
        }
    }
};

#endif