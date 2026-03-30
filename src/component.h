#include <vector>
#include <unordered_map>


struct Component {
    virtual ~Component() {}
};

struct Rigidbody : Component {
    float mass = 0;
};


/*
struct Hierarchy {
    int parent = -1;                 // -1 = brak rodzica
    std::vector<int> children;       // lista dzieci
};

struct Rigidbody {
    float mass = 0;
};
*/

/*
class Entity {
public:
    int id;
    int parent;

    Entity(int id) : id(id) {}
};
*/

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

};


/*
class System {
public:
    virtual ~System() {}
    virtual void Update(std::vector<Entity*>& entities) = 0;
};*/

class System {
public:
    virtual ~System() {}
    virtual void Update(ECS& ecs) = 0;


    void RegisterEntity(Entity* e) { registeredEntities.push_back(e); }
};


class MovementSystem : public System {
public:
    void Update(ECS& ecs) override {
        for (auto* e : ecs.GetEntities()) {
            auto* t = e->GetComponent<Transform>();
            auto* v = e->GetComponent<Velocity>();
            if (t && v) {
                t->x += v->vx;
                t->y += v->vy;
            }
        }
    }
};

/*
class MovementSystem : public System {
public:
    void Update(ECS& ecs) {
        for (auto& e : ecs.GetEntities()) {

            auto* t = ecs.GetComponent<Transform>(e->id);
            auto* v = ecs.GetComponent<Velocity>(e->id);

            if (t && v) {
                t->x += v->vx;
                t->y += v->vy;
            }
        }
    }
};*/

/*
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
*/

/*
struct IComponentStorage {
    virtual ~IComponentStorage() = default;
};


template<typename T>
class ComponentStorage : public IComponentStorage {
public:
    std::unordered_map<int, T> data;
};
*/


class ECS {
private:
    std::vector<std::unique_ptr<Entity>> entities;
    std::vector<std::unique_ptr<System>> systems;

public:
    Entity* CreateEntity() {
        Entity* e = new Entity(entities.size());
        entities.emplace_back(e);
        return e;
    }

    std::vector<Entity*> GetEntities() {
        std::vector<Entity*> result;
        for (auto& e : entities)
            result.push_back(e.get());
        return result;
    }

    template<typename T, typename... Args>
    T* AddSystem(Args&&... args) {
        T* sys = new T(std::forward<Args>(args)...);
        systems.emplace_back(sys);
        return sys;
    }

    void Update() {
        for (auto& s : systems)
            s->Update(*this);
    }
};


/*
class ECS {
private:
    std::unordered_map<std::type_index, std::unique_ptr<IComponentStorage>> components;
    std::vector<std::unique_ptr<Entity>> entities;
    std::vector<std::unique_ptr<System>> systems;

    //std::vector<std::unique_ptr<Entity>> entities;
    //std::vector<std::unique_ptr<System>> systems;
    //int nextID = 0;

public:

    Entity* CreateEntity() {
        Entity* e = new Entity();
        entities.emplace_back(e);
        return e;
    }

    template<typename T>
    void RegisterComponent() {
        std::type_index type = typeid(T);
        components[type] = std::make_unique<ComponentStorage<T>>();
    }

    template<typename T>
    void AddComponent(int entityID, const T& component) {
        auto storage = static_cast<ComponentStorage<T>*>(components[typeid(T)].get());
        storage->data[entityID] = component;
    }

    template<typename T>
    T* GetComponent(int entityID) {
        auto storage = static_cast<ComponentStorage<T>*>(components[typeid(T)].get());

        auto it = storage->data.find(entityID);
        if (it != storage->data.end())
            return &it->second;

        return nullptr;
    }

    template<typename T>
    void RemoveComponent(int entityID) {
        auto storage = static_cast<ComponentStorage<T>*>(components[typeid(T)].get());
        storage->data.erase(entityID);
    }


    template<typename T, typename... Args>
    T* AddSystem(Args&&... args) {
        T* sys = new T(std::forward<Args>(args)...);
        systems.emplace_back(sys);
        return sys;
    }

    void Update() {
        for (auto& s : systems)
            s->Update(*this);
    }

    /*
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
};*/


int main() {
    ECS ecs;

    ecs.RegisterComponent<Rigidbody>();

    Entity* e = ecs.CreateEntity();

    ecs.AddComponent<Rigidbody>(e->id, { 0, 0 });

    ecs.AddSystem<MovementSystem>();

    ecs.Update();

    return 0;
}