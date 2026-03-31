#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include <unordered_map>
#include <typeindex>
#include <vector>
#include <algorithm>
#include <utility>
#include "component.h"


class ECS;

class GameObject {

private:
    ECS* ecs;
    GameObject* parent = nullptr;

    std::unordered_map<std::type_index, std::vector<Component*>> componentMap; //Typ komponentu i vektor tego typu komponentu

public:

    GameObject(ECS* ecs_ptr) : ecs(ecs_ptr) {}

    template<typename T, typename... Args>
    T* AddComponent(Args&&... args) {
        T* comp = new T(std::forward<Args>(args)...);
        componentMap[typeid(T)].push_back(comp);
        NotifyChanged();
        return GetComponent<T>();
    }

    template<typename T>
    T* GetComponent() {
        auto it = componentMap.find(typeid(T));
        if (it != componentMap.end() && !it->second.empty())
            return static_cast<T*>(it->second[0]); // pierwszy komponent danego typu
        return nullptr;
    }

    template<typename T>
    std::vector<T*> GetComponents() {
        std::vector<T*> result;
        auto it = componentMap.find(typeid(T));
        if (it != componentMap.end()) {
            for (Component* c : it->second)
                result.push_back(static_cast<T*>(c));
        }
        return result;
    }

    template<typename T>
    void RemoveComponent(T* component) {
        auto it = componentMap.find(typeid(T));
        if (it != componentMap.end()) {
            auto& vec = it->second;
            auto vecIt = std::find(vec.begin(), vec.end(), component);
            if (vecIt != vec.end()) {
                delete* vecIt;
                vec.erase(vecIt);
                NotifyChanged();
            }
            if (vec.empty()) componentMap.erase(it);
        }
    }

    void NotifyChanged();

    ~GameObject();

};

#endif


/*
template<typename T, typename... Args>
T* AddComponent(Args&&... args) {
    static_assert(std::is_base_of<Component, T>::value, "Must inherit Component");

    T* comp = new T(std::forward<Args>(args)...);
    components.emplace_back(comp);
    return comp;
}*/

/*
template<typename T, typename... Args>
T* AddComponent(Args&&... args) {
    T* comp = new T(std::forward<Args>(args)...);
    components.emplace_back(comp);

    ecs->NotifyEntityChanged(this);

    return comp;
}

template<typename T>
T* GetComponent() {
    for (auto& c : components) {
        if (auto casted = dynamic_cast<T*>(c.get()))
            return casted;
    }
    return nullptr;
}
*/