#include "gameobject.h"
#include "ecs.h"

void GameObject::NotifyChanged() {
    ecs->NotifyGameObjectChanged(this);
}

GameObject::~GameObject() {
    // Tylko wyczyœæ mapê wskaŸników — nie usuwamy fizycznie komponentów
    componentMap.clear();
}
