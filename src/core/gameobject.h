#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include <unordered_map>
#include <typeindex>
#include <vector>
#include <algorithm>
#include <utility>
#include "component.h"
#include "component_pool.h"

class ECS;

class GameObject {
public:
    size_t id;
    uint64_t componentMask = 0;

private:
    static size_t nextId;

    ECS* ecs;
    GameObject* parent = nullptr;

    //std::unordered_map<std::type_index, std::vector<Component*>> componentMap; //Typ komponentu i vektor tego typu komponentu
    std::unordered_map<std::type_index, std::vector<Component*>> componentMap;

public:

    GameObject(ECS* ecs_ptr) : ecs(ecs_ptr), id(nextId++) {}

    template<typename T, typename... Args>
    T* AddComponent(Args&&... args) {
        static_assert(std::is_base_of<Component, T>::value, "T must inherit Component");

        T* comp = GetPool<T>().Allocate();
        *comp = T(std::forward<Args>(args)...);
        componentMap[std::type_index(typeid(T))].push_back(comp);

        componentMask |= T::ComponentBit;
        NotifyChanged();

        return GetComponent<T>();
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
                if (vec.empty()) {
                    componentMap.erase(it);
                    componentMask &= ~T::ComponentBit;
                }
                NotifyChanged();
                return;
            }
        }
    }


    //template<typename T, typename... Args>
    //T* AddComponent(Args&&... args);

    //template<typename T>
    //T* GetComponent();
    //
    //template<typename T>
    //std::vector<T*> GetComponents();
    //
    //template<typename T>
    //void RemoveComponent(T* component);

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