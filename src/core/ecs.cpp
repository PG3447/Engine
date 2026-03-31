#include "ecs.h"

void ECS::NotifyGameObjectChanged(GameObject* e) {
    for (auto& q : queries)
        q->OnGameObjectUpdated(e);

    for (auto& sys : systems)
        sys->OnGameObjectUpdated(e);
}


void ECS::Update() {
    for (auto& sys : systems)
        sys->Update(*this);
}


GameObject* ECS::CreateGameObject() {
    GameObject* e = new GameObject(this);
    gameobjects.emplace_back(e);
    NotifyGameObjectChanged(e);
    return e;
}

ECS::~ECS() = default;