#include <vector>
#include <unordered_map>


struct Component {
    virtual ~Component() {}
};

struct Rigidbody : Component {
    float mass = 0;
};

class Entity {
public:
    int id;

    Entity(int id) : id(id) {}
};

/*
class Entity {

private:
    std::vector<std::unique_ptr<Component>> components;


public:
    template<typename T, typename... Args>
    T* AddComponent(Args&&... args) {
        static_assert(std::is_base_of<Component, T>::value, "Must inherit Component");

        T* comp = new T(std::forward<Args>(args)...);
        components.emplace_back(comp);
        return comp;
    }

    template<typename T>
    T* GetComponent() {
        for (auto& c : components) {
            if (auto casted = dynamic_cast<T*>(c.get()))
                return casted;
        }
        return nullptr;
    }

};*/


class System {
public:
    virtual ~System() {}
    virtual void Update(std::vector<Entity*>& entities) = 0;
};


class MovementSystem {
    std::vector<Entity*> validEntities; // tylko encje z Transform i Velocity

public:
    void RegisterEntity(Entity* e) {
        if (e->GetComponent<Transform>() && e->GetComponent<Velocity>()) {
            validEntities.push_back(e);
        }
    }

    void Update() {
        for (auto* e : validEntities) {
            auto* t = e->GetComponent<Transform>();
            auto* v = e->GetComponent<Velocity>();
            t->x += v->vx;
            t->y += v->vy;
        }
    }
};



class World {
private:
    std::vector<std::unique_ptr<Entity>> entities;
    std::vector<std::unique_ptr<System>> systems;
    //int nextID = 0;

public:
    //std::unordered_map<int, Transform> transforms;
    //std::unordered_map<int, Velocity> velocities;

    Entity* CreateEntity() {
        Entity* e = new Entity();
        entities.emplace_back(e);
        return e;
    }

    // Dodawanie systemów
    template<typename T, typename... Args>
    T* AddSystem(Args&&... args) {
        T* sys = new T(std::forward<Args>(args)...);
        systems.emplace_back(sys);

        // zarejestruj wszystkie istniejące encje
        for (auto& e : entities)
            sys->RegisterEntity(e.get());

        return sys;
    }

    // Aktualizacja
    void Update() {
        for (auto& s : systems)
            s->Update();
    }
};





int main() {
    World world;

    world.AddSystem<MovementSystem>();

    auto* player = world.CreateEntity();
    player->AddComponent<Transform>();
    auto* vel = player->AddComponent<Velocity>();

    vel->vx = 1.0f;
    vel->vy = 0.5f;

    for (int i = 0; i < 5; i++) {
        world.Update();
    }

    return 0;
}
