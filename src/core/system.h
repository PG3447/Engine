#ifndef SYSTEM_H
#define SYSTEM_H

class ECS;
class GameObject;
struct GLFWwindow;

class System {
public:
    virtual ~System() {}

    virtual void InformedActiveECS(ECS& ecs, GLFWwindow* win = nullptr) {}

    virtual void Update(ECS& ecs, float deltaTime) = 0;

    virtual void OnGameObjectUpdated(GameObject* e) = 0;

};


#endif