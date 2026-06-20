#ifndef ECS_H
#define ECS_H

#include <memory>
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <vector>
#include <algorithm>
#include <utility>
#include "gameobject.h"
#include "system.h"
#include "query.h"


class ECS {
private:
    std::vector<std::unique_ptr<GameObject>> gameobjects;
    std::vector<std::shared_ptr<System>> systems;
    std::vector<std::unique_ptr<QueryBase>> queries;


public:

    template<typename... Components>
    Query<Components...>* CreateQuery() {
        auto* q = new Query<Components...>();
        for (auto& e : gameobjects)
            q->OnGameObjectUpdated(e.get());
        queries.emplace_back(q);
        return q;
    }

    template<typename T, typename... Args>
    T* AddSystem(Args&&... args) {
        T* sys = new T(std::forward<Args>(args)...);
        systems.emplace_back(sys);
        for (auto& e : gameobjects)
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

    void AddExistingSystem(System* sys) {
        if (!sys) return;
        for (auto& e : gameobjects)
            sys->OnGameObjectUpdated(e.get());
        systems.push_back(std::shared_ptr<System>(sys, [](System*) {}));
    }

    void InformActiveECS(GLFWwindow* window)
    {
        for (auto& sys : systems) {
            sys->InformedActiveECS(*this, window);
        }
    }

    //void AddSystems(std::vector<std::shared_ptr<System>> sysList)
    //{
    //    for (auto& sys : sysList) {
    //        for (auto& e : gameobjects)
    //            sys->OnGameObjectUpdated(e.get());
    //        systems.emplace_back(std::move(sys));
    //    }
    //}

    void NotifyGameObjectChanged(GameObject* e);

    GameObject* CreateGameObject();

    std::vector<GameObject*> GetAllGameObjects() {
        std::vector<GameObject*> result;
        result.reserve(gameobjects.size());
        for (auto& obj : gameobjects) {
            result.push_back(obj.get());
        }
        return result;
    }

    ~ECS();
};

#endif