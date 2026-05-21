#ifndef PREFAB_H
#define PREFAB_H

#include <entity.h>
#include <shader.h>
#include <string>
#include <memory>
#include "resource_manager.h"
#include "core/scene.h"

class Prefab
{
public:
    std::shared_ptr<Model> rootModel;

    Prefab(std::string const& path)
    {
        if (!path.empty()) {
            rootModel = ResourceManager::LoadModel(path); // Magia optymalizacji
        }
    }

    GameObject* Instantiate(Scene& scene, GameObject* parent = nullptr, Shader* shader = nullptr)
    {
        if (!rootModel || !rootModel->rootNode) return nullptr;

        bool isAnimatedModel = rootModel->skeleton.boneCount > 0;

        return CreateRecursive(&scene, rootModel->rootNode.get(), parent, shader, nullptr, true, isAnimatedModel);
    }

private:

    GameObject* CreateRecursive(Scene* scene, ModelNode* model, GameObject* parent, Shader* shader,
        AnimatorComponent* rootAnimator, bool isRoot, bool isAnimated)
    {
        if (!model) return nullptr;

        GameObject* go = scene->CreateGameObject(parent);
        go->name = model->name;

        AnimatorComponent* currentAnimator = rootAnimator;

        if (isRoot && isAnimated) {
            currentAnimator = go->AddComponent<AnimatorComponent>();
            currentAnimator->currentSkeleton = &rootModel->skeleton;
        }

        auto* transform = go->GetComponent<TransformComponent>();
        if (transform) {
            if (currentAnimator != nullptr) {
                transform->position = glm::vec3(0.0f);
                transform->rotation = glm::vec3(0.0f);
                transform->scale = glm::vec3(1.0f);
            }
            else {
                transform->position = model->transform.getLocalPosition();
                transform->rotation = model->transform.getLocalRotation();
                transform->scale = model->transform.getLocalScale();
            }
            transform->isDirty = true;
        }

        auto* render = go->AddComponent<RenderComponent>();
        render->meshes = model->meshes;

        for (auto& mesh : render->meshes) {
            // Nie nadpisuj globalnych materiałów, gdy nie przekazano shadera.
            if (mesh.material && shader) {
                mesh.material->shader = shader;
            }
        }

        for (auto& child : model->children) {
            CreateRecursive(scene, child.get(), go, shader, currentAnimator, false, isAnimated);
        }

        return go;
    }

};

#endif // PREFAB_H