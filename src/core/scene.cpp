#include "scene.h"
#include <spdlog/spdlog.h>

#include "systems/PostProcessingSystem.h"
#include "systems/NavMeshSystem.h"
#include "systems/NavPathSystem.h"
#include "systems/NpcSystem.h"
#include "systems/raycastSystem.h"
#include "systems/AudioSystem.h"
#include "systems/UI_system.h"


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
    if (auto* hid = ecs.GetSystem<HID>())
        hid->Update(ecs, deltaTime);

    if (auto* ui = ecs.GetSystem<UISystem>())
        ui->Update(ecs, deltaTime);

    if (auto* ps = ecs.GetSystem<PhysicsSystem>())
        ps->Update(ecs, deltaTime);    

    if (auto* ts = ecs.GetSystem<TransformSystem>())
        ts->updateSelfAndChild(root.get());

    if (auto* ps = ecs.GetSystem<AnimationSystem>())
        ps->Update(ecs, deltaTime);

    if (auto* rs = ecs.GetSystem<RaycastSystem>())
        rs->Update(ecs, deltaTime);

    if (auto* render = ecs.GetSystem<RenderSystem>())
        render->Update(ecs, deltaTime);

    if (auto* obj = ecs.GetSystem<NavMeshSystem>()) {
        obj->Update(ecs, deltaTime);
    }
    if (auto* obj = ecs.GetSystem<NavPathSystem>()) {
        obj->Update(ecs, deltaTime);
    }

    if (auto* ns = ecs.GetSystem<NpcSystem>())
        ns->Update(ecs, deltaTime);

    if (auto* pps = ecs.GetSystem<PostProcessingSystem>())
        pps->Update(ecs, deltaTime);

    if (auto* ss = ecs.GetSystem<SpriteSystem>())
        ss->Update(ecs, deltaTime);



    //if (auto* as = ecs.GetSystem<AudioSystem>())
        //as->Update(ecs, deltaTime);

}
//
//std::vector<GameObject*> Scene::GetGameObjects() {
//    return ecs.GetGameObjects();
//}

void Scene::addAllSystems(GLFWwindow* window) {
    ecs.AddSystem<TransformSystem>(ecs);
    ecs.AddSystem<PhysicsSystem>(ecs);
    ecs.AddSystem<AnimationSystem>(ecs);
    ecs.AddSystem<RenderSystem>(ecs, window);
    ecs.AddSystem<HID>(ecs, window);
    ecs.AddSystem<PostProcessingSystem>(ecs, window);
    ecs.AddSystem<SpriteSystem>(ecs, window);
    ecs.AddSystem<RaycastSystem>(ecs);
    ecs.AddSystem<NavMeshSystem>(ecs);
    ecs.AddSystem<NavPathSystem>(ecs);
    ecs.AddSystem<AudioSystem>(ecs);
    ecs.AddSystem<NpcSystem>(ecs);
}

void Scene::DebugHierarchy(GameObject* obj, int depth)
{
    if (!obj) return;

    std::string indent(depth * 2, ' ');

    spdlog::info("{}GameObject: {}", indent, (void*)obj);
    spdlog::info("GameObject: {}", obj->name);

    auto* t = obj->GetComponent<TransformComponent>();
    if (t)
    {
        spdlog::info("{}  pos: {:.2f}, {:.2f}, {:.2f}",
            indent, t->position.x, t->position.y, t->position.z);
    }

    for (auto* child : obj->GetChildren())
    {
        DebugHierarchy(child, depth + 1);
    }
}