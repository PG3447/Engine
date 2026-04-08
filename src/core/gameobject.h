#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include <unordered_map>
#include <typeindex>
#include <vector>
#include <algorithm>
#include <utility>
#include "component.h"

class ECS;


template<typename T>
class ComponentPool {
private:
    std::vector<T> pool;
    std::vector<size_t> freeIndices;

public:
    T* Allocate() {
        if (!freeIndices.empty()) {
            size_t idx = freeIndices.back();
            freeIndices.pop_back();
            return &pool[idx];
        }
        pool.emplace_back();
        return &pool.back();
    }

    void Free(T* comp) {
        size_t index = comp - pool.data();
        if (index < pool.size()) {
            *comp = T(); // reset
            freeIndices.push_back(index);
        }
    }

    void Clear() {
        pool.clear();
        freeIndices.clear();
    }

    size_t Size() const { return pool.size(); }
};


template<typename T>
static ComponentPool<T>& GetPool() {
    static ComponentPool<T> pool;
    return pool;
}

class GameObject {
public:
    size_t id;
private:
    ECS* ecs;
    GameObject* parent = nullptr;

    //std::unordered_map<std::type_index, std::vector<Component*>> componentMap; //Typ komponentu i vektor tego typu komponentu
    std::unordered_map<std::type_index, std::vector<Component*>> componentMap;

public:

    GameObject(ECS* ecs_ptr) : ecs(ecs_ptr) {}

    template<typename T, typename... Args>
    T* AddComponent(Args&&... args) {
        static_assert(std::is_base_of<Component, T>::value, "T must inherit Component");

        // alokacja komponentu
        T* comp = GetPool<T>().Allocate();
        // wywo≥anie konstruktora w miejscu
        new (comp) T(std::forward<Args>(args)...);
        // zapis wskaünika w mapie
        componentMap[std::type_index(typeid(T))].push_back(comp);
        return comp;
    }

    template<typename T>
    T* GetComponent() {
        auto it = componentMap.find(std::type_index(typeid(T)));
        if (it != componentMap.end() && !it->second.empty())
            return static_cast<T*>(it->second[0]);
        return nullptr;
    }

    template<typename T>
    std::vector<T*> GetComponents() {
        std::vector<T*> result;
        auto it = componentMap.find(std::type_index(typeid(T)));
        if (it != componentMap.end()) {
            for (auto* c : it->second)
                result.push_back(static_cast<T*>(c));
        }
        return result;
    }


    template<typename T>
    void RemoveComponent(T* comp = nullptr) {
        auto it = componentMap.find(std::type_index(typeid(T)));
        if (it == componentMap.end() || it->second.empty()) return;

        auto& vec = it->second;
        if (!comp) comp = static_cast<T*>(vec.back());

        for (size_t i = 0; i < vec.size(); ++i) {
            if (vec[i] == comp) {
                vec[i] = vec.back();
                vec.pop_back();
                GetPool<T>().Free(comp);
                if (vec.empty()) componentMap.erase(it);
                return;
            }
        }
    }


    //template<typename T, typename... Args>
    //T* AddComponent(Args&&... args);

    template<typename T>
    T* GetComponent();

    template<typename T>
    std::vector<T*> GetComponents();

    template<typename T>
    void RemoveComponent(T* component);

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