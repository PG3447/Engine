#ifndef PREFAB_H
#define PREFAB_H

#include <entity.h>
#include <model.h>
#include <shader.h>
#include <string>
#include <memory>
#include "resource_manager.h"

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

    Entity* getEntitiesCreate(Shader* shader, Light* light = nullptr)
    {
        if (!rootModel) return nullptr;
        return createEntityRecursive(rootModel.get(), nullptr, shader, light);
    }

    Entity* createEntityRecursive(Model* model, Entity* parent, Shader* shader, Light* light)
    {
        if (!model) return nullptr;

        Entity* entity = new Entity();
        entity->transform = model->transform;
        entity->pModel = model;
        entity->pShader = shader;
        entity->pLight = light;
        entity->name = model->name;

        if (parent)
            parent->addChild(entity);

        for (auto& childModel : model->children)
            createEntityRecursive(&childModel, entity, shader, light);

        return entity;
    }
};
#endif