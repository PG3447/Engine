#pragma once
#include "ECS.h"


template<typename T, typename... Args>
T* GameObject::AddComponent(Args&&... args) {
    auto typeIdx = std::type_index(typeid(T));

    if (!ecs->componentStores.count(typeIdx))
        ecs->componentStores[typeIdx] = std::make_unique<ComponentStorage<T>>();

    auto* storage = static_cast<ComponentStorage<T>*>(ecs->componentStores[typeIdx].get());

    // dodaj komponent w storage
    storage->components.emplace_back(std::forward<Args>(args)...);

    T* compPtr = &storage->components.back();

    // przypisz tylko jeœli jest unikalny dla tego GameObject
    componentMap[typeIdx].push_back(compPtr);

    ecs->NotifyGameObjectChanged(this);
    return GetComponent<T>();
}

template<typename T>
T* GameObject::GetComponent() {
    auto it = componentMap.find(typeid(T));
    if (it != componentMap.end() && !it->second.empty())
        return static_cast<T*>(it->second[0]); // pierwszy komponent danego typu
    return nullptr;
}

template<typename T>
std::vector<T*> GameObject::GetComponents() {
    std::vector<T*> result;
    auto it = componentMap.find(typeid(T));
    if (it != componentMap.end()) {
        for (Component* c : it->second)
            result.push_back(static_cast<T*>(c));
    }
    return result;
}

template<typename T>
void GameObject::RemoveComponent(T* component) {
    auto it = componentMap.find(typeid(T));
    if (it != componentMap.end()) {
        auto& vec = it->second;
        auto vecIt = std::find(vec.begin(), vec.end(), component);
        if (vecIt != vec.end()) {
            vec.erase(vecIt);
            ecs->NotifyGameObjectChanged(this);
            //NotifyChanged();
        }
        if (vec.empty()) componentMap.erase(it);
    } 
}