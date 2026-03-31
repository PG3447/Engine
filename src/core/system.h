#ifndef SYSTEM_H
#define SYSTEM_H

class ECS;
class GameObject;

class System {
public:
    virtual ~System() {}

    virtual void Update(ECS& ecs) = 0;

    virtual void OnGameObjectUpdated(GameObject* e) = 0;

};


#endif