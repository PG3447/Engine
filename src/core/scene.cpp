#include "scene.h"
#include <spdlog/spdlog.h>


//Scene::Scene(ECS& ecsRef) : ecs(ecsRef) {}

GameObject* Scene::CreateGameObject(GameObject* parent) {
    GameObject* e = ecs.CreateGameObject();


    // Dodanie TransformComponent, jeśli brak
    auto* t = e->GetComponent<TransformComponent>();
    if (!t) {
        spdlog::info("dodano transform");
        t = e->AddComponent<TransformComponent>();
    } 

    if (!parent)
        parent = root.get();

    parent->AddChild(e);

    ecs.NotifyGameObjectChanged(e);
    return e;
}

void Scene::Update(float deltaTime) {
    // kolejność ważna: transform → fizyka → render
    if (root.get() != nullptr) {
        spdlog::info("Istnieje root");
        //DebugHierarchy(root.get());
    }

    if (auto* hid = ecs.GetSystem<HID>()) hid->Update(ecs);
    if (auto* ts = ecs.GetSystem<TransformSystem>()) ts->updateSelfAndChild(root.get());
    if (auto* ps = ecs.GetSystem<PhysicsSystem>()) ps->Update(ecs);
    if (auto* render = ecs.GetSystem<RenderSystem>()) render->Update(ecs);
    if (auto* ss = ecs.GetSystem<SpriteSystem>()) ss->Update(ecs);
}
//
//std::vector<GameObject*> Scene::GetGameObjects() {
//    return ecs.GetGameObjects();
//}


void Scene::DebugHierarchy(GameObject* obj, int depth)
{
    if (!obj) return;

    std::string indent(depth * 2, ' ');

    //spdlog::info("{}GameObject: {}", indent, (void*)obj);

    auto* t = obj->GetComponent<TransformComponent>();
    if (t)
    {
        //spdlog::info("{}  pos: {:.2f}, {:.2f}, {:.2f}",
        //    indent, t->position.x, t->position.y, t->position.z);
    }

    for (auto* child : obj->GetChildren())
    {
        DebugHierarchy(child, depth + 1);
    }
}