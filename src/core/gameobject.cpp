#include "gameobject.h"
#include "ecs.h"

size_t GameObject::nextId = 0;

void GameObject::NotifyChanged() {
    ecs->NotifyGameObjectChanged(this);
}

GameObject::~GameObject() {
    // Tylko wyczyœæ mapê wskaŸników — nie usuwamy fizycznie komponentów
    componentMap.clear();
}
