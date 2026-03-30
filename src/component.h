#include <vector>
#include <unordered_map>


struct Component {
    virtual ~Component() {}
};

struct Rigidbody : Component {
    float mass = 0;
    float vx = 0, vy = 0;
};

struct Transform : Component {
    float x = 0, y = 0;
};

class Entity {

private:
    ECS* ecs;
    Entity* parent = null;
    //std::vector<std::unique_ptr<Component>> components;
    std::unordered_map<std::type_index, Component*> componentMap;


public:
    /*
    template<typename T, typename... Args>
    T* AddComponent(Args&&... args) {
        static_assert(std::is_base_of<Component, T>::value, "Must inherit Component");

        T* comp = new T(std::forward<Args>(args)...);
        components.emplace_back(comp);
        return comp;
    }*/

    /*
    template<typename T, typename... Args>
    T* AddComponent(Args&&... args) {
        T* comp = new T(std::forward<Args>(args)...);
        components.emplace_back(comp);

        ecs->NotifyEntityChanged(this);

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
    */

    template<typename T, typename... Args>
    T* AddComponent(Args&&... args) {
        T* comp = new T(std::forward<Args>(args)...);
        componentMap[typeid(T)].push_back(comp);
        ecs->NotifyEntityChanged(this);
        return comp;
    }

    template<typename T>
    T* GetComponent() {
        auto it = componentMap.find(typeid(T));
        return it != componentMap.end() ? static_cast<T*>(it->second) : nullptr;
    }

    template<typename T>
    std::vector<T*> GetComponents() {
        std::vector<T*> result;
        auto it = componentMap.find(typeid(T));
        if (it != componentMap.end()) {
            for (Component* c : it->second)
                result.push_back(static_cast<T*>(c));
        }
        return result;
    }

    template<typename T>
    void RemoveComponent(T* component) {
        auto it = componentMap.find(typeid(T));
        if (it != componentMap.end()) {
            auto& vec = it->second;
            auto vecIt = std::find(vec.begin(), vec.end(), component);
            if (vecIt != vec.end()) {
                delete* vecIt;
                vec.erase(vecIt);
                ecs->NotifyEntityChanged(this);
            }
            if (vec.empty()) componentMap.erase(it);
        }
    }


    ~Entity() {
        for (auto& [type, comp] : componentMap) {
            delete comp;
        }
        componentMap.clear();
    }
};



class System {
/*
protected:
    std::vector<Entity*> registeredEntities;
    */

public:
    virtual ~System() {}

    //virtual bool Matches(Entity* e) = 0;

    virtual void Update(ECS& ecs) = 0;

    virtual void OnEntityUpdated(Entity* e) = 0;

    /*
    void TryRegister(Entity* e) {
        if (!Matches(e)) return;

        auto it = std::find(registeredEntities.begin(), registeredEntities.end(), e);
        if (it == registeredEntities.end()) {
            registeredEntities.push_back(e);
        }
    }*/

};



class MovementSystem : public System {
private:
    std::vector<Transform*> transforms;
    std::vector<Velocity*> velocities;

public:
    void OnEntityUpdated(Entity* e) override {

        auto* t = e->GetComponent<Transform>();
        auto* v = e->GetComponent<Velocity>();

        if (t && v) {
            // unikanie duplikatow
            auto it = std::find(transforms.begin(), transforms.end(), t);
            if (it == transforms.end()) {
                transforms.push_back(t);
                velocities.push_back(v);
            }
        }
    }

    void Update(ECS& ecs) override {
        for (size_t i = 0; i < transforms.size(); i++) {
            transforms[i]->x += velocities[i]->vx;
            transforms[i]->y += velocities[i]->vy;
        }
    }
};

class PhysicsSystem : public System {
private:
    std::vector<Transform*> transforms;
    std::vector<Rigidbody*> rigidbodies;

public:
    void OnEntityUpdated(Entity* e) override {

        auto* t = e->GetComponent<Transform>();
        auto* rb = e->GetComponent<Rigidbody>();

        if (t && rb) {
            // unikamy duplikatów po Transform*
            auto it = std::find(transforms.begin(), transforms.end(), t);
            if (it == transforms.end()) {
                transforms.push_back(t);
                rigidbodies.push_back(rb);
            }
        }
    }


    void Update(ECS& ecs) override {
        for (size_t i = 0; i < transforms.size(); i++) {
            float ay = -9.8f; // grawitacja

            rigidbodies[i]->vy += ay; // aktualizacja prędkości
            transforms[i]->matrix += rigidbodies[i]->vy; // aktualizacja pozycji
        }
    }

    void ApplyForce(Entity* e, float fx, float fy) {
        auto* rb = e->GetComponent<Rigidbody>();
        if (rb) {
            rb->vx += fx;
            rb->vy += fy;
        }
    }
};

class ECS {
private:
    std::vector<std::unique_ptr<Entity>> entities;
    std::vector<std::unique_ptr<System>> systems;

public:
    Entity* CreateEntity() {
        Entity* e = new Entity(entities.size(), this);
        entities.emplace_back(e);

        RegisterEntityToSystems(e);

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

        for (auto& e : entities) {
            sys->OnEntityUpdated(e.get());
        }

        return sys;
    }

    void Update() {
        for (auto& s : systems)
            s->Update(*this);
    }

    /*
    void RegisterEntityToSystems(Entity* e) {
        for (auto& sys : systems) {
            sys->TryRegister(e);
        }
    }*/

    void NotifyEntityChanged(Entity* e) {
        for (auto& sys : systems) {
            sys->OnEntityUpdated(e);
        }
    }
};


int main() {
    ECS ecs;

    ecs.RegisterComponent<Rigidbody>();

    Entity* e = ecs.CreateEntity();

    ecs.AddComponent<Rigidbody>(e->id, { 0, 0 });

    ecs.AddSystem<MovementSystem>();

    ecs.Update();

    return 0;
}

/*
class MovementSystem : public System {
public:
    bool Matches(Entity* e) override {
        return e->GetComponent<Transform>() &&
            e->GetComponent<Velocity>();
    }

    void Update(ECS& ecs) override {
        for (auto* e : registeredEntities) {
            auto* t = e->GetComponent<Transform>();
            auto* v = e->GetComponent<Velocity>();

            t->x += v->vx;
            t->y += v->vy;
        }
    }
};
*/

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


/*
class System {
public:
    virtual ~System() {}
    virtual void Update(std::vector<Entity*>& entities) = 0;
};*/


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

