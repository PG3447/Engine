#ifndef SCENE_H
#define SCENE_H

#include <vector>
#include "ecs.h"
#include "../systems/physics_system.h"
#include "../systems/transform_system.h"
#include "../systems/render_system.h"

class Scene {
private:
    ECS& ecs;
    std::unique_ptr<GameObject> root;

public:
    Scene(ECS& ecsRef) : ecs(ecsRef)
    {
        root = std::make_unique<GameObject>(&ecs);
        root->AddComponent<TransformComponent>();
    }

    GameObject* GetRoot() { return root.get(); }
    
    GameObject* CreateGameObject(GameObject* parent = nullptr);

    // Tworzy encję i od razu dodaje komponenty
    template<typename... Components>
    GameObject* CreateEntityWithComponents(GameObject* parent = nullptr) {
        GameObject* e = CreateGameObject(parent);
        (e->AddComponent<Components>(), ...);
        ecs.NotifyGameObjectChanged(e);
        return e;
    }

    // Tworzenie query w ECS
    template<typename... Components>
    Query<Components...>* CreateQuery() {
        return ecs.CreateQuery<Components...>();
    }

    void Update(float deltaTime);


    void DebugHierarchy(GameObject* obj, int depth = 0);
    //std::vector<GameObject*> GetGameObjects();

    ECS& GetECS() { return ecs; }
};


#endif