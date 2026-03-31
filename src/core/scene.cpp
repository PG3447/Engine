#include "scene.h"

Scene::Scene(ECS& ecsRef) : ecs(ecsRef) {}

GameObject* Scene::CreateGameObject(GameObject* parent) {
    GameObject* e = ecs.CreateGameObject();

    // Ustawienie relacji w hierarchii (jeœli encja ma Transform)
    auto* t = e->GetComponent<Transform>();
    if (!t) t = e->AddComponent<Transform>();

    if (parent) {
        auto* pt = parent->GetComponent<Transform>();
        if (pt) t->parent = pt; // ustawienie rodzica w Transform
    }

    return e;
}

void Scene::Update(float deltaTime) {
    ecs.Update();
}

std::vector<GameObject*> Scene::GetEntities() {
    return ecs.GetEntities();
}