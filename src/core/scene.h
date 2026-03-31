#ifndef SCENE_H
#define SCENE_H

#include <vector>
#include "ecs.h"

class Scene {
private:
    ECS& ecs;

public:
    Scene(ECS& ecsRef);

    // opcjonalnie ustawiaj¹c rodzica
    GameObject* CreateGameObject(GameObject* parent = nullptr);

    // Tworzy encjê i od razu dodaje komponenty
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

    //std::vector<GameObject*> GetGameObjects();

    ECS& GetECS() { return ecs; }
};


#endif