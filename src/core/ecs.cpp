#include "ecs.h"
#include <spdlog/spdlog.h>

void ECS::NotifyGameObjectChanged(GameObject* e) {
    for (auto& q : queries)
        q->OnGameObjectUpdated(e);

    for (auto& sys : systems)
        sys->OnGameObjectUpdated(e);
}


GameObject* ECS::CreateGameObject() {
    GameObject* e = new GameObject(this);
    gameobjects.emplace_back(e);
    NotifyGameObjectChanged(e);
    return e;
}

void ECS::DestroyGameObject(GameObject* e) {
    if (!e) return;

    if (GameObject* parent = e->GetParent()) {
        parent->RemoveChild(e);
    }

    auto it = std::find_if(gameobjects.begin(), gameobjects.end(),
        [e](const std::unique_ptr<GameObject>& up) { return up.get() == e; });

    if (it != gameobjects.end()) {
        NotifyGameObjectChanged(e);
        gameobjects.erase(it);
        spdlog::info("ECS::DestroyGameObject: usunieto gameobject");
    } else {
        spdlog::warn("ECS::DestroyGameObject: nie istnieje taki gameobject - nie mozna usunac");
    }
}

ECS::~ECS() = default;