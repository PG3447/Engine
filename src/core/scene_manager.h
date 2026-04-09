#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#include "scene.h"


class SceneManager {
private:
    std::unordered_map<std::string, std::unique_ptr<Scene>> scenes;
    Scene* activeScene = nullptr;

public:
    SceneManager() = default;

    // Tworzy now¹ scenê i dodaje do managera
    Scene* CreateScene(const std::string& name, ECS& ecs) {
        auto scene = std::make_unique<Scene>(ecs);
        Scene* ptr = scene.get();
        scenes[name] = std::move(scene);
        if (!activeScene) activeScene = ptr;
        return ptr;
    }

    // Aktywuje scenê o danej nazwie
    void SetActiveScene(const std::string& name) {
        auto it = scenes.find(name);
        if (it != scenes.end()) activeScene = it->second.get();
    }

    // Pobiera aktywn¹ scenê
    Scene* GetActiveScene() { return activeScene; }

    // Update wywo³uje update tylko aktywnej sceny
    void Update(float deltaTime) {
        if (activeScene) activeScene->Update(deltaTime);
    }


    // Opcjonalnie usuwa scenê
    void DestroyScene(const std::string& name) {
        if (activeScene == scenes[name].get()) activeScene = nullptr;
        scenes.erase(name);
    }
};

#endif