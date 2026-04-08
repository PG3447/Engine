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
    std::vector<std::unique_ptr<System>> systems;
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


//#include "GameObject.tpp"


#endif


//std::unordered_map<std::type_index, std::unique_ptr<ComponentStorageBase>> componentStores;


/*
struct ComponentStorageBase {
    virtual ~ComponentStorageBase() = default;
};

template<typename T>
struct ComponentStorage : ComponentStorageBase {
    std::vector<T> components; // wszystkie komponenty typu T w jednym wektorze
};
*/

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