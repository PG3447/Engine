#include "gameobject.h"
#include "ecs.h"


void GameObject::NotifyChanged() {
    ecs->NotifyGameObjectChanged(this);
}

GameObject::~GameObject() {
    for (auto& [type, comps] : componentMap) {
        for (Component* c : comps)
            delete c;
    }
    componentMap.clear();
}
