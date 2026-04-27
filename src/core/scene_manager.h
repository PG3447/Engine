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