#include "GameObject.h"

void GameObject::RemoveComponent(Component* component) {
    if (!component) return;

    for (auto it = componentMap.begin(); it != componentMap.end(); ++it) {
        auto& vec = it->second;
        auto vecIt = std::find(vec.begin(), vec.end(), component);
        if (vecIt != vec.end()) {
            delete* vecIt;
            vec.erase(vecIt);
            if (ecs) ecs->NotifyEntityChanged(this);
            if (vec.empty()) componentMap.erase(it);
            return;
        }
    }
}

GameObject::~GameObject() {
    for (auto& [type, comps] : componentMap) {
        for (Component* c : comps)
            delete c;
    }
    componentMap.clear();
}