#ifndef ECS_H
#define ECS_H

#include <memory>
#include <vector>
#include "gameobject.h"
#include "system.h"
#include "component.h"
#include "query.h"


class ECS {
private:
    std::vector<std::unique_ptr<GameObject>> gameObjects;
    std::vector<std::unique_ptr<System>> systems;
    std::vector<std::unique_ptr<QueryBase>> queries;

public:
    template<typename... Components>
    Query<Components...>* CreateQuery() {
        auto* q = new Query<Components...>();
        for (auto& e : gameObjects)
            q->OnGameObjectUpdated(e.get());
        queries.emplace_back(q);
        return q;
    }

    template<typename T, typename... Args>
    T* AddSystem(Args&&... args) {
        T* sys = new T(std::forward<Args>(args)...);
        systems.emplace_back(sys);
        for (auto& e : gameObjects)
            sys->OnGameObjectUpdated(e.get());
        return sys;
    }

    template<typename T>
    T* GetSystem() {
        for (auto& sys : systems) {
            if (auto casted = dynamic_cast<T*>(sys.get()))
                return casted;
        }
        return nullptr;
    }

    void NotifyGameObjectChanged(GameObject* e);

    void Update();

    GameObject* CreateGameObject();
};

#endif


/*
template<typename T, typename... Args>
T* AddSystemSingleton() {
    // sprawdzamy, czy system ju¿ istnieje
    for (auto& s : systems) {
        if (dynamic_cast<T*>(s.get())) return static_cast<T*>(s.get());
    }

    // tworzymy nowy
    T* sys = new T(std::forward<Args>(args)...);
    systems.emplace_back(sys);
    return sys;
}*/