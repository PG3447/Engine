#include "scene.h"

//Scene::Scene(ECS& ecsRef) : ecs(ecsRef) {}

GameObject* Scene::CreateGameObject(GameObject* parent) {
    GameObject* e = ecs.CreateGameObject();

    // Dodanie TransformComponent, jeœli brak
    auto* t = e->GetComponent<TransformComponent>();
    if (!t) t = e->AddComponent<TransformComponent>();

    return e;
}

void Scene::Update(float deltaTime) {
    ecs.Update();
}
//
//std::vector<GameObject*> Scene::GetGameObjects() {
//    return ecs.GetGameObjects();
//}