#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#include "scene.h"


class SceneManager {
private:
    std::unordered_map<std::string, std::unique_ptr<Scene>> scenes;
    Scene* activeScene = nullptr;
public:
    
    SceneManager() = default;

    Scene* CreateScene(const std::string& name)
    {
        auto scene = std::make_unique<Scene>();
        Scene* ptr = scene.get();
        scenes[name] = std::move(scene);
        if (!activeScene) activeScene = ptr;
        return ptr;
    }

    void SetActiveScene(const std::string& name, GLFWwindow* window)
    {
        auto it = scenes.find(name);
        if (it != scenes.end())
        {
            activeScene = it->second.get();
            activeScene->GetECS().InformActiveECS(window);
        }

    }

    Scene* GetActiveScene() { return activeScene; }

    Scene* GetScene(const std::string& name)
    {
        auto it = scenes.find(name);
        if (it != scenes.end()) return it->second.get();
        return nullptr;
    }

    void Update(float deltaTime) {
        if (activeScene) activeScene->Update(deltaTime);
    }

    void Load()
    {
        Scene* scene = GetActiveScene();
        ECS& ecs = scene->GetECS();

        YamlConfig cfg;
        if (!cfg.load("scene.yaml"))
            return;
        
        YAML::Node sceneNode = cfg.getRoot();

        if (!sceneNode["Scene"] || !sceneNode["Scene"]["GameObjects"])
            return;
        
        YAML::Node objectsNode = sceneNode["Scene"]["GameObjects"];

        std::unordered_map<size_t, GameObject*> idMap;

        for (GameObject* obj : ecs.GetAllGameObjects())
        {
            idMap[obj->id] = obj;
        }

        // 1. TWORZENIE OBIEKTÓW + KOMPONENTY
        for (auto objNode : objectsNode)
        {
            size_t id = objNode["id"].as<size_t>();

            auto found = idMap.find(id);
            if (found == idMap.end())
                continue;

            GameObject* obj = found->second;

            //update components
            YAML::Node compsNode = objNode["Components"];

            for (auto compIt : compsNode)
            {
                YAML::Node compNode = compIt;

                std::string type = compNode["type"].as<std::string>();

                // znajdź istniejący komponent
                if (type == "Transform" || type == "Light" || type == "Collider")
                {
                    Component* comp = obj->GetComponentByName(type);
                    if (!comp)
                        continue;
                    comp->Deserialize(compNode);
                }


            }
        }

        // 2. ODTWORZENIE HIERARCHII (parent-child)
        //for (auto it : objectsNode)
        //{
        //    YAML::Node objNode = it.second;

        //    int id = objNode["id"].as<int>();
        //    int parentId = objNode["parent"].as<int>();

        //    GameObject* obj = idMap[id];

        //    if (!obj)
        //        continue;

        //    if (parentId != -1 && idMap.count(parentId))
        //    {
        //        obj->SetParent(idMap[parentId]);
        //    }
        //    else
        //    {
        //        obj->SetParent(nullptr);
        //    }
        //}
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

        for (GameObject* obj : objects)
        {
            YAML::Node objNode;
            SaveGameObject(objNode, obj);

            sceneNode["Scene"]["GameObjects"].push_back(objNode);
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
            compNode["bit"] = comp->ComponentBit;
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