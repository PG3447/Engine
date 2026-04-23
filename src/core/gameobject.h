#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include <unordered_map>
#include <typeindex>
#include <vector>
#include <algorithm>
#include <utility>
#include "component.h"
#include "component_pool.h"

class ECS;

class GameObject {
public:
    size_t id;
    uint64_t componentMask = 0;

private:
    static size_t nextId;

    ECS* ecs;
    GameObject* parent = nullptr;
    std::vector<GameObject*> children;

    //std::unordered_map<std::type_index, std::vector<Component*>> componentMap; //Typ komponentu i vektor tego typu komponentu
    std::unordered_map<std::type_index, std::vector<Component*>> componentMap;

public:

    GameObject(ECS* ecs_ptr, GameObject* parent_ptr = nullptr)
        : ecs(ecs_ptr), id(nextId++)
    {
        if (parent_ptr)
            parent_ptr->AddChild(this);
    }

    template<typename T, typename... Args>
    T* AddComponent(Args&&... args) {
        static_assert(std::is_base_of<Component, T>::value, "T must inherit Component");

        T* comp = GetPool<T>().Allocate();
        *comp = T(std::forward<Args>(args)...);
        componentMap[std::type_index(typeid(T))].push_back(comp);

        componentMask |= T::ComponentBit;
        NotifyChanged();

        return GetComponent<T>();
    }

    template<typename T>
    T* GetComponent() {
        auto it = componentMap.find(std::type_index(typeid(T)));
        if (it != componentMap.end() && !it->second.empty())
            return static_cast<T*>(it->second[0]);
        return nullptr;
    }

    template<typename T>
    std::vector<T*> GetComponents() {
        std::vector<T*> result;
        auto it = componentMap.find(std::type_index(typeid(T)));
        if (it != componentMap.end()) {
            for (auto* c : it->second)
                result.push_back(static_cast<T*>(c));
        }
        return result;
    }


    template<typename T>
    void RemoveComponent(T* comp = nullptr) {
        auto it = componentMap.find(std::type_index(typeid(T)));
        if (it == componentMap.end() || it->second.empty()) return;

        auto& vec = it->second;
        if (!comp) comp = static_cast<T*>(vec.back());

        for (size_t i = 0; i < vec.size(); ++i) {
            if (vec[i] == comp) {
                vec[i] = vec.back();
                vec.pop_back();
                GetPool<T>().Free(comp);
                if (vec.empty()) {
                    componentMap.erase(it);
                    componentMask &= ~T::ComponentBit;
                }
                NotifyChanged();
                return;
            }
        }
    }

    template<typename T>
    std::vector<T*> GetComponentsInChildren() {
        std::vector<T*> result;

        if (T* comp = GetComponent<T>()) {
            result.push_back(comp);
        }

        for (GameObject* child : children) {
            std::vector<T*> childComps = child->GetComponentsInChildren<T>();
            result.insert(result.end(), childComps.begin(), childComps.end());
        }

        return result;
    }

    void AddChild(GameObject* child) {
        if (!child || child == this) return;

        // jeśli miał parenta → usuń z niego
        if (child->parent) {
            child->parent->RemoveChild(child);
        }

        child->parent = this;
        children.push_back(child);

        child->NotifyChanged();
    }

    void RemoveChild(GameObject* child) {
        if (!child) return;

        auto it = std::find(children.begin(), children.end(), child);
        if (it != children.end()) {
            (*it)->parent = nullptr;

            // swap & pop (wydajne)
            *it = children.back();
            children.pop_back();

            child->NotifyChanged();
        }
    }

    void SetParent(GameObject* newParent) {
        if (parent == newParent) return;

        // usuń z obecnego parenta
        if (parent) {
            parent->RemoveChild(this);
        }

        parent = newParent;

        if (newParent) {
            newParent->children.push_back(this);
        }

        NotifyChanged();
    }

    bool HasChildren() const {
        return !children.empty();
    }

    GameObject* FindChild(GameObject* target) {
        auto it = std::find(children.begin(), children.end(), target);
        return (it != children.end()) ? *it : nullptr;
    }


    GameObject* GetParent() const { return parent; }

    const std::vector<GameObject*>& GetChildren() const { return children; }

    //template<typename T, typename... Args>
    //T* AddComponent(Args&&... args);

    //template<typename T>
    //T* GetComponent();
    //
    //template<typename T>
    //std::vector<T*> GetComponents();
    //
    //template<typename T>
    //void RemoveComponent(T* component);

    void NotifyChanged();

    ~GameObject();

};


#endif


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