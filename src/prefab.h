#ifndef PREFAB_H
#define PREFAB_H

#include <entity.h>
#include <shader.h>
#include <string>
#include <memory>
#include "resource_manager.h"
#include "core/scene.h"

using namespace std;

class Prefab
{
public:
    std::shared_ptr<Model> rootModel;

    Prefab(string const& path)
    {
        if (!path.empty()) {
            rootModel = ResourceManager::LoadModel(path); // Magia optymalizacji
        }
    }


    //Entity* getEntitiesCreate(Shader* shader, Light* light = nullptr)
    //{
    //    if (!rootModel) return nullptr;
    //    return createEntityRecursive(rootModel.get(), nullptr, shader, light);
    //}

    GameObject* Instantiate(Scene& scene, GameObject* parent = nullptr, Shader* shader = nullptr)
    {
        if (!rootModel) return nullptr;
         
        return CreateRecursive(&scene, rootModel.get(), parent, shader);
    }

private:

    //Entity* createEntityRecursive(Model* model, Entity* parent, Shader* shader, Light* light)
    //{
    //    if (!model) return nullptr;

    //    Entity* entity = new Entity();
    //    entity->transform = model->transform;
    //    entity->pModel = model;
    //    entity->pShader = shader;
    //    entity->pLight = light;
    //    entity->name = model->name;

    //    if (parent)
    //        parent->addChild(entity);

    //    for (auto& childModel : model->children)
    //        createEntityRecursive(&childModel, entity, shader, light);

    //    return entity;
    //}


    GameObject* CreateRecursive(Scene* scene, Model* model, GameObject* parent, Shader* shader)
    {
        if (!model) return nullptr;

        GameObject* go = scene->CreateGameObject(parent);

        // Transform
        auto* transform = go->AddComponent<TransformComponent>();
        transform->position = model->transform.getLocalPosition();
        transform->rotation = model->transform.getLocalRotation();
        transform->scale = model->transform.getLocalScale();
        transform->modelMatrix = model->transform.getModelMatrix();
        transform->isDirty = true;

        // Render
        auto* render = go->AddComponent<RenderComponent>();
        render->model = model;
        render->shader = shader;

        if (model && !model->nodes.empty()) {
            render->materialOverride = model->nodes[0].material;
        }
        else
        {
            render->materialOverride = std::make_shared<Material>(shader);
        }

       

        // (opcjonalnie światło jeśli model ma)
        //if (model->hasLight) {
        //    auto* light = go->AddComponent<LightComponent>();
        //    light->data = model->lightData;
        //}

        // nazwa (jeśli masz pole)
        // go->name = model->name;

        // ===== DZIECI =====
        for (auto& child : model->children) {
            CreateRecursive(scene, &child, go, shader);
        }

        return go;
    }
};
#endif




