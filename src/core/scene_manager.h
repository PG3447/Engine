#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#include "scene.h"


class SceneManager {
private:
    std::unordered_map<std::string, std::unique_ptr<Scene>> scenes;
    Scene* activeScene = nullptr;

public:
    SceneManager() = default;

    Scene* CreateScene(const std::string& name, ECS& ecs) {
        auto scene = std::make_unique<Scene>(ecs);
        Scene* ptr = scene.get();
        scenes[name] = std::move(scene);
        if (!activeScene) activeScene = ptr;
        return ptr;
    }

    void SetActiveScene(const std::string& name) {
        auto it = scenes.find(name);
        if (it != scenes.end()) activeScene = it->second.get();
    }

    Scene* GetActiveScene() { return activeScene; }

    void Update(float deltaTime) {
        if (activeScene) activeScene->Update(deltaTime);
    }

    void Load()
    {
        //for (auto compNode : node["Components"])
        //{
        //    std::string type = compNode["type"].as<std::string>();

        //    Component* comp = CreateComponentFromType(type);

        //    comp->Deserialize(compNode);
        //    obj->Attach(comp);
        //}
    }

    void Save()
    {
        Scene* scene = GetActiveScene();

        ECS& ecs = scene->GetECS();

        YamlConfig cfg;

        auto objects = ecs.GetAllGameObjects();

        YAML::Node sceneNode;

        int i = 0;

        for (GameObject* obj : objects)
        {
            YAML::Node objNode;
            SaveGameObject(objNode, obj);

            sceneNode["Scene"]["GameObjects"][i++] = objNode;
        }


        cfg.getRoot() = sceneNode;
        cfg.save("scene.yaml");
    }


    void SaveGameObject(YAML::Node& node, GameObject* obj)
    {
        node["id"] = obj->id;
        node["name"] = obj->name;

        node["parent"] = obj->GetParent() ? obj->GetParent()->id : -1;

        YAML::Node compsNode;

        for (Component* comp : obj->GetAllComponents())
        {
            YAML::Node compNode;

            compNode["type"] = comp->GetTypeName();
            comp->Serialize(compNode);

            compsNode.push_back(compNode);
        }

        node["Components"] = compsNode;
    }

    //void SaveGameObject(YAML::Node& node, GameObject* obj)
    //{
    //    node["id"] = obj->id;
    //    node["name"] = obj->name;

    //    node["parent"] =
    //        obj->GetParent()
    //        ? obj->GetParent()->id
    //        : -1;
    //
    //    if (auto* t = obj->GetComponent<TransformComponent>())
    //    {
    //        node["Transform"]["position"] =
    //            TransformHelper::getLocalPosition(*t);

    //        node["Transform"]["rotation"] =
    //            TransformHelper::getLocalRotation(*t);

    //        node["Transform"]["scale"] =
    //            TransformHelper::getLocalScale(*t);
    //    }

    //    if (auto* c = obj->GetComponent<ColliderComponent>())
    //    {
    //        node["Collider"]["offset"] = c->offset;
    //        node["Collider"]["halfSize"] = c->halfSize;
    //        node["Collider"]["isTrigger"] = c->isTrigger;
    //    }
    //}


    // Opcjonalnie usuwa scen�
    void DestroyScene(const std::string& name) {
        auto it = scenes.find(name);
        if (it != scenes.end()) {
            if (activeScene == it->second.get())
                activeScene = nullptr;
            scenes.erase(it);
        }
    }
};

#endif