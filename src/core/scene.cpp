#include "scene.h"


//Scene::Scene(ECS& ecsRef) : ecs(ecsRef) {}

GameObject* Scene::CreateGameObject(GameObject* parent) {
    GameObject* e = ecs.CreateGameObject();

    // Dodanie TransformComponent, jeśli brak
    auto* t = e->GetComponent<TransformComponent>();
    if (!t) t = e->AddComponent<TransformComponent>();

    return e;
}

void Scene::Update(float deltaTime) {
    // kolejność ważna: transform → fizyka → render
    if (auto* rs = ecs.GetSystem<HID>()) rs->Update(ecs);
    if (auto* ts = ecs.GetSystem<TransformSystem>()) ts->updateSelfAndChild(root.get());
    if (auto* ps = ecs.GetSystem<PhysicsSystem>()) ps->Update(ecs);
    if (auto* rs = ecs.GetSystem<RenderSystem>()) rs->Update(ecs);
}
//
//std::vector<GameObject*> Scene::GetGameObjects() {
//    return ecs.GetGameObjects();
//}