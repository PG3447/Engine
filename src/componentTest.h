//
//
//
//
//
//
//
//
//void imgui_begin()
//{
//    // Start the Dear ImGui frame
//    ImGui_ImplOpenGL3_NewFrame();
//    ImGui_ImplGlfw_NewFrame();
//    ImGui::NewFrame();
//}
////
//////Wszystko ponizej to imgui
////static Entity* selectedEntity = nullptr;
////
////void showLightEditor(Light& light)
////{
////    ImGui::Checkbox("Light Enabled", &light.isOn);
////
////    const char* lightTypes[] = { "Directional", "Point", "Spot" };
////    int type = static_cast<int>(light.type);
////    if (ImGui::Combo("Light Type", &type, lightTypes, IM_ARRAYSIZE(lightTypes)))
////    {
////        light.type = static_cast<Light::LightType>(type);
////    }
////
////    ImGui::Separator();
////    ImGui::Text("Colors");
////
////    ImGui::ColorEdit3("Ambient", &light.ambient.x);
////    ImGui::ColorEdit3("Diffuse", &light.diffuse.x);
////    ImGui::ColorEdit3("Specular", &light.specular.x);
////
////    ImGui::Separator();
////
////    if (light.type != Light::Directional)
////    {
////        ImGui::DragFloat3("Position", &light.position.x, 0.05f);
////    }
////
////    if (light.type == Light::Directional || light.type == Light::Spot)
////    {
////        ImGui::DragFloat3("Direction", &light.direction.x, 0.05f);
////    }
////
////    if (light.type == Light::Point || light.type == Light::Spot)
////    {
////        ImGui::Separator();
////        ImGui::Text("Attenuation");
////
////        ImGui::DragFloat("Constant", &light.constant, 0.01f, 0.0f, 2.0f);
////        ImGui::DragFloat("Linear", &light.linear, 0.001f, 0.0f, 1.0f);
////        ImGui::DragFloat("Quadratic", &light.quadratic, 0.001f, 0.0f, 1.0f);
////    }
////
////    if (light.type == Light::Spot)
////    {
////        ImGui::Separator();
////        ImGui::Text("Spotlight Angles");
////
////        float inner = glm::degrees(acos(light.cutOff));
////        float outer = glm::degrees(acos(light.outerCutOff));
////
////        if (ImGui::SliderFloat("Inner CutOff", &inner, 0.0f, 90.0f))
////            light.cutOff = glm::cos(glm::radians(inner));
////
////        if (ImGui::SliderFloat("Outer CutOff", &outer, inner, 90.0f))
////            light.outerCutOff = glm::cos(glm::radians(outer));
////    }
////}
////
////
////void showTransformEditor(Transform& transform)
////{
////    glm::vec3 pos = transform.getLocalPosition();
////    glm::vec3 rot = transform.getLocalRotation();
////    glm::vec3 scale = transform.getLocalScale();
////
////    if (ImGui::DragFloat3("Position", &pos.x, 0.01f))
////    {
////        change = true;
////        transform.setLocalPosition(pos);
////    }
////
////    if (ImGui::DragFloat3("Rotation", &rot.x, 0.1f))
////    {
////        change = true;
////        transform.setLocalRotation(rot);
////    }
////
////    if (ImGui::DragFloat3("Scale", &scale.x, 0.01f, 0.01f))
////    {
////        change = true;
////        transform.setLocalScale(scale);
////    }
////}
////static ImGuiTextFilter entityFilter;
////
////bool entityMatchesFilter(Entity* entity)
////{
////    if (entityFilter.PassFilter(entity->name.c_str()))
////        return true;
////
////    for (auto& child : entity->children)
////    {
////        if (entityMatchesFilter(child.get()))
////            return true;
////    }
////    return false;
////}
////
////
////void showEntityTree(Entity* entity)
////{
////    if (!entity) return;
////
////    if (!entityMatchesFilter(entity))
////        return;
////
////    ImGuiTreeNodeFlags flags =
////        ImGuiTreeNodeFlags_OpenOnArrow |
////        ImGuiTreeNodeFlags_OpenOnDoubleClick |
////        ((entity == selectedEntity) ? ImGuiTreeNodeFlags_Selected : 0);
////
////    bool nodeOpen = ImGui::TreeNodeEx(
////        (void*)entity,
////        flags,
////        "%s",
////        entity->name.c_str()
////    );
////
////    if (ImGui::IsItemClicked())
////        selectedEntity = entity;
////
////    if (nodeOpen)
////    {
////        for (auto& child : entity->children)
////            showEntityTree(child.get());
////
////        ImGui::TreePop();
////    }
////}
////
//
//void imgui_render()
//{
//    if (show_demo_window)
//    {
//        //ImGui::ShowDemoWindow(&show_demo_window);
//    }
//
//    ImGui::Begin("Hello, world!");
//
//    if (ImGui::Button(wireframeMode ? "Switch to Fill Mode" : "Switch to Wireframe"))
//    {
//        wireframeMode = !wireframeMode;
//        glPolygonMode(GL_FRONT_AND_BACK,
//            wireframeMode ? GL_LINE : GL_FILL);
//    }
//
//    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
//    ImGui::SliderFloat("rotation X", &rotationX, -480.0f, 480.0f);
//    ImGui::SliderFloat("rotation Y", &rotationY, -480.0f, 480.0f);
//    ImGui::SliderFloat("Camera Distance", &cameraDistance, 5.0f, 1000.0f);
//
//    ImGui::Text("This is some useful text.");
//    ImGui::Checkbox("Demo Window", &show_demo_window);
//    ImGui::Checkbox("Another Window", &show_another_window);
//
//    ImGui::ColorEdit3("clear color", (float*)&clear_color);
//
//    ImGui::End(); // 🔥 MUSI BYĆ
//
//    if (show_another_window)
//    {
//        ImGui::Begin("Another Window", &show_another_window);
//        ImGui::Text("Hello from another window!");
//        if (ImGui::Button("Close Me"))
//            show_another_window = false;
//        ImGui::End();
//    }
//}
////
////void imgui_render()
////{
////    /// Add new ImGui controls here
////    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
////    if (show_demo_window)
////
////        //ImGui::ShowDemoWindow(&show_demo_window);
////
////    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
////
////    {
////        ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.
////
////
////        if (ImGui::Button(wireframeMode ? "Switch to Fill Mode" : "Switch to Wireframe"))
////        {
////            wireframeMode = !wireframeMode;
////            if (wireframeMode)
////                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // włącz wireframe
////            else
////                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // normalny render
////        }
////
////        ImGui::SliderFloat("rotation X", &rotationX, -480.0f, 480.0f);
////        ImGui::SliderFloat("rotation Y", &rotationY, -480.0f, 480.0f);
////        ImGui::SliderFloat("Camera Distance", &cameraDistance, 5.0f, 1000.0f);
////
////
////        ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
////        ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
////        ImGui::Checkbox("Another Window", &show_another_window);
////
////        ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color
////    }
////    //    if (ImGui::Button("Auto rotation"))
////    //        autoRotation = !autoRotation;
////    //    ImGui::SameLine();
////    //    ImGui::Text(" = %s", autoRotation ? "true" : "false");
////
////    //    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
////    //    
////    //   if (root) // główny obiekt sceny
////    //   {
////    //       ImGui::Separator();
////    //       ImGui::Text("Hierarchy");
////    //       entityFilter.Draw("Search", 200);
////    //       ImGui::Separator();
////
////    //       showEntityTree(root.get());
////
////    //   
////    //       if (selectedEntity)
////    //       {
////    //           ImGui::Separator();
////    //           ImGui::Text("Selected Entity: %s", selectedEntity->name.c_str());
////    //           showTransformEditor(selectedEntity->transform);
////    //       }
////
////
////    //       ImGui::Separator();
////    //       ImGui::Text("Lighting");
////
////    //       if (ImGui::CollapsingHeader("Directional Light"))
////    //       {
////    //           ImGui::PushID("Directional");
////    //           showLightEditor(*dircetLight);
////    //           ImGui::PopID();
////    //       }
////
////    //       //if (ImGui::CollapsingHeader("Point Lights"))
////    //       //{
////    //       //    ImGui::PushID("pointLight");
////    //       //    showLightEditor(*pointLight);
////    //       //    ImGui::PopID();
////    //       //}
////
////    //       //if (ImGui::CollapsingHeader("Spot Light"))
////    //       //{
////    //       //    ImGui::PushID("spotLight");
////    //       //    showLightEditor(*spotLight);
////    //       //    ImGui::PopID();
////    //       //}
////
////    //       //if (ImGui::CollapsingHeader("Spot Light2"))
////    //       //{
////    //       //    ImGui::PushID("spotLight2");
////    //       //    showLightEditor(*spotLight2);
////    //       //    ImGui::PopID();
////    //       //}
////    //   }
////    //   
////    //    
////    //    ImGui::End();
////    //}
////
////    // 3. Show another simple window.
////    if (show_another_window)
////    {
////        ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
////        ImGui::Text("Hello from another window!");
////        if (ImGui::Button("Close Me"))
////            show_another_window = false;
////        ImGui::End();
////    }
////}
//
//void imgui_end()
//{
//    ImGui::Render();
//    int display_w, display_h;
//    glfwMakeContextCurrent(window);
//    glfwGetFramebufferSize(window, &display_w, &display_h);
//
//    glViewport(0, 0, display_w, display_h);
//
//
//    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
//}
//
//
//
//












//#include <iostream>
//#include <vector>
//#include <unordered_map>
//#include <typeindex>
//#include <memory>
//
//// ================= Components =================
//struct Component { virtual ~Component() = default; };
//
//struct TransformComponent : Component {
//    float x = 0, y = 0;
//};
//
//struct RigidbodyComponent : Component {
//    float vx = 0, vy = 0;
//};
//
//// ================= Forward =================
//class GameObject;
//class Scene;
//class ECS;
//
//// ================= Component Storage =================
//template<typename T>
//class ComponentStorage {
//public:
//    std::unordered_map<int, T> components; // ID obiektu -> komponent
//
//    void Add(int id, const T& comp) {
//        components[id] = comp;
//    }
//
//    void Remove(int id) {
//        components.erase(id);
//    }
//
//    T* Get(int id) {
//        auto it = components.find(id);
//        if (it != components.end()) return &it->second;
//        return nullptr;
//    }
//};
//
//// ================= ECS =================
//class ECS {
//private:
//    std::unordered_map<std::type_index, std::unique_ptr<ComponentStorageBase>> storageBase;
//
//public:
//    template<typename T>
//    ComponentStorage<T>& GetOrCreateStorage() {
//        std::type_index idx(typeid(T));
//        if (storageBase.find(idx) == storageBase.end()) {
//            storageBase[idx] = std::make_unique<ComponentStorage<T>>();
//        }
//        return *static_cast<ComponentStorage<T>*>(storageBase[idx].get());
//    }
//
//    template<typename T>
//    void AddComponent(int objectID, const T& comp) {
//        GetOrCreateStorage<T>().Add(objectID, comp);
//    }
//
//    template<typename T>
//    void RemoveComponent(int objectID) {
//        GetOrCreateStorage<T>().Remove(objectID);
//    }
//
//    template<typename T>
//    T* GetComponent(int objectID) {
//        return GetOrCreateStorage<T>().Get(objectID);
//    }
//};
//
//// ================= GameObject =================
//class GameObject {
//public:
//    int id;
//    GameObject* parent = nullptr;
//    std::vector<GameObject*> children;
//
//    GameObject(int id_, GameObject* parent_ = nullptr) : id(id_), parent(parent_) {
//        if (parent) parent->children.push_back(this);
//    }
//};
//
//// ================= Scene =================
//class Scene {
//private:
//    int nextID = 0;
//public:
//    std::vector<std::unique_ptr<GameObject>> objects;
//
//    GameObject* CreateObject(GameObject* parent = nullptr) {
//        auto obj = std::make_unique<GameObject>(nextID++, parent);
//        GameObject* ptr = obj.get();
//        objects.push_back(std::move(obj));
//        return ptr;
//    }
//};
//
//// ================= Example =================
//int main() {
//    Scene scene;
//    ECS ecs;
//
//    GameObject* root = scene.CreateObject();
//    GameObject* child = scene.CreateObject(root);
//
//    ecs.AddComponent(root->id, TransformComponent{ 10, 20 });
//    ecs.AddComponent(root->id, TransformComponent{ 30, 40 });
//    ecs.AddComponent(root->id, RigidbodyComponent{ 1, 2 });
//
//    // Pobieranie komponentów
//    TransformComponent* t1 = ecs.GetComponent<TransformComponent>(root->id);
//    if (t1) std::cout << "Transform: x=" << t1->x << ", y=" << t1->y << "\n";
//
//    RigidbodyComponent* rb = ecs.GetComponent<RigidbodyComponent>(root->id);
//    if (rb) std::cout << "Rigidbody: vx=" << rb->vx << ", vy=" << rb->vy << "\n";
//
//    return 0;
//}
//

/*#include <iostream>
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <memory>

// ================= Components =================
struct Component { virtual ~Component() = default; };

struct TransformComponent : Component {
    float x = 0, y = 0;
};

struct RigidbodyComponent : Component {
    float vx = 0, vy = 0;
};

// ================= Forward =================
class GameObject;
class ECS;

// ================= Component Storage =================
template<typename T>
struct ComponentStorage {
    std::vector<T> components;

    // Dodaj komponent
    T* Add(const T& comp) {
        components.push_back(comp);
        return &components.back();
    }

    // Usuń komponent swap-pop
    void Remove(size_t index) {
        if (index >= components.size()) return;
        std::swap(components[index], components.back());
        components.pop_back();
    }

    size_t Size() const { return components.size(); }
};

// ================= ECS =================
struct ComponentStorageBase { virtual ~ComponentStorageBase() = default; };

class ECS {
private:
    std::unordered_map<std::type_index, std::unique_ptr<ComponentStorageBase>> componentStores;
    std::vector<std::unique_ptr<GameObject>> gameobjects;

public:
    GameObject* CreateGameObject(GameObject* parent = nullptr);

    template<typename T>
    ComponentStorage<T>* GetOrCreateStorage() {
        std::type_index typeIdx(typeid(T));
        auto it = componentStores.find(typeIdx);
        if (it != componentStores.end())
            return static_cast<ComponentStorage<T>*>(it->second.get());

        auto storage = std::make_unique<ComponentStorage<T>>();
        ComponentStorage<T>* ptr = storage.get();
        componentStores[typeIdx] = std::move(storage);
        return ptr;
    }

    template<typename T>
    T* AddComponent(GameObject* obj, const T& comp);

    template<typename T>
    void RemoveComponent(GameObject* obj, size_t index);
};

// ================= GameObject =================
class GameObject {
public:
    GameObject* parent = nullptr;
    std::vector<GameObject*> children;

    // Indeksy komponentów w storage
    std::unordered_map<std::type_index, std::vector<size_t>> componentIndices;

    GameObject(GameObject* parentPtr = nullptr) : parent(parentPtr) {
        if (parent) parent->children.push_back(this);
    }

    template<typename T>
    std::vector<T*> GetComponents(ECS& ecs) {
        std::vector<T*> result;
        auto it = componentIndices.find(typeid(T));
        if (it == componentIndices.end()) return result;

        auto* storage = ecs.GetOrCreateStorage<T>();
        for (size_t idx : it->second)
            result.push_back(&storage->components[idx]);
        return result;
    }
};

// ================= Implementacja ECS =================
GameObject* ECS::CreateGameObject(GameObject* parent) {
    auto obj = std::make_unique<GameObject>(parent);
    GameObject* ptr = obj.get();
    gameobjects.push_back(std::move(obj));
    return ptr;
}

template<typename T>
T* ECS::AddComponent(GameObject* obj, const T& comp) {
    auto* storage = GetOrCreateStorage<T>();
    T* c = storage->Add(comp);
    obj->componentIndices[typeid(T)].push_back(storage->components.size() - 1);
    return c;
}

template<typename T>
void ECS::RemoveComponent(GameObject* obj, size_t index) {
    auto* storage = GetOrCreateStorage<T>();
    if (index >= storage->Size()) return;

    storage->Remove(index);

    auto& vec = obj->componentIndices[typeid(T)];
    vec.erase(std::remove(vec.begin(), vec.end(), index), vec.end());

    // Aktualizacja indeksów po swap-pop
    for (auto& pair : obj->componentIndices) {
        for (auto& i : pair.second)
            if (i > index) i--;
    }
}

// ================= Example =================
int main() {
    ECS ecs;

    GameObject* root = ecs.CreateGameObject();
    GameObject* child = ecs.CreateGameObject(root);

    ecs.AddComponent<TransformComponent>(root, { 10, 20 });
    ecs.AddComponent<TransformComponent>(root, { 30, 40 });
    ecs.AddComponent<RigidbodyComponent>(root, { 1, 2 });

    auto transforms = root->GetComponents<TransformComponent>(ecs);
    for (auto* t : transforms)
        std::cout << "Root Transform: x=" << t->x << ", y=" << t->y << "\n";

    return 0;
}

//#include <iostream>
//#include <vector>
//#include <unordered_map>
//#include "transform.h"
//
//struct Component {
//    virtual ~Component() {}
//};
//
//struct Rigidbody : Component {
//    float mass = 0;
//    float vx = 0, vy = 0;
//};
//
//
//
//
//struct TransformComponent : Component {
//    glm::vec3 position{ 0.0f, 0.0f, 0.0f };
//    glm::vec3 rotation{ 0.0f, 0.0f, 0.0f };
//    glm::vec3 scale{ 1.0f, 1.0f, 1.0f };
//
//    glm::mat4 modelMatrix{ 1.0f };
//    bool isDirty = true;
//};
//
//struct MonoBehaviour : Component {
//    virtual ~MonoBehaviour() {}
//
//    virtual void Start() {}
//    virtual void Update(float deltaTime) {}
//};
//
//class Entity {
//
//private:
//    ECS* ecs;
//    Entity* parent = null;
//    //std::vector<std::unique_ptr<Component>> components;
//    std::unordered_map<std::type_index, std::vector<Component*>> componentMap; //Typ komponentu i vektor tego typu komponentu
//
//
//public:
//  
//    template<typename T, typename... Args>
//    T* AddComponent(Args&&... args) {
//        T* comp = new T(std::forward<Args>(args)...);
//        componentMap[typeid(T)].push_back(comp);
//        ecs->NotifyEntityChanged(this);
//        return comp;
//    }
//
//    template<typename T>
//    T* GetComponent() {
//        auto it = componentMap.find(typeid(T));
//        if (it != componentMap.end() && !it->second.empty())
//            return static_cast<T*>(it->second[0]); // pierwszy komponent danego typu
//        return nullptr;
//    }
//
//    template<typename T>
//    std::vector<T*> GetComponents() {
//        std::vector<T*> result;
//        auto it = componentMap.find(typeid(T));
//        if (it != componentMap.end()) {
//            for (Component* c : it->second)
//                result.push_back(static_cast<T*>(c));
//        }
//        return result;
//    }
//
//    template<typename T>
//    void RemoveComponent(T* component) {
//        auto it = componentMap.find(typeid(T));
//        if (it != componentMap.end()) {
//            auto& vec = it->second;
//            auto vecIt = std::find(vec.begin(), vec.end(), component);
//            if (vecIt != vec.end()) {
//                delete* vecIt;
//                vec.erase(vecIt);
//                ecs->NotifyEntityChanged(this);
//            }
//            if (vec.empty()) componentMap.erase(it);
//        }
//    }
//
//    ~Entity() {
//        for (auto& [type, comps] : componentMap) {
//            for (Component* c : comps)
//                delete c;
//        }
//        componentMap.clear();
//    }
//
//    /*
//  template<typename T, typename... Args>
//  T* AddComponent(Args&&... args) {
//      static_assert(std::is_base_of<Component, T>::value, "Must inherit Component");
//
//      T* comp = new T(std::forward<Args>(args)...);
//      components.emplace_back(comp);
//      return comp;
//  }*/
//
//  /*
//  template<typename T, typename... Args>
//  T* AddComponent(Args&&... args) {
//      T* comp = new T(std::forward<Args>(args)...);
//      components.emplace_back(comp);
//
//      ecs->NotifyEntityChanged(this);
//
//      return comp;
//  }
//
//  template<typename T>
//  T* GetComponent() {
//      for (auto& c : components) {
//          if (auto casted = dynamic_cast<T*>(c.get()))
//              return casted;
//      }
//      return nullptr;
//  }
//  */
//
//};
//
//
//
//struct TransformComponent : Component {
//    glm::vec3 position{ 0.0f, 0.0f, 0.0f };
//    glm::vec3 rotation{ 0.0f, 0.0f, 0.0f };
//    glm::vec3 scale{ 1.0f, 1.0f, 1.0f };
//
//    glm::mat4 modelMatrix{ 1.0f };
//    bool isDirty = true;
//};
//
//
//class QueryBase {
//public:
//    virtual ~QueryBase() {}
//    virtual void OnEntityUpdated(Entity* e) = 0;
//};
//
//template<typename... Components>
//class Query : public QueryBase {
//public:
//    // Struktura SoA – oddzielne wektory komponentów i encji
//    std::vector<Entity*> entities;
//    std::tuple<std::vector<Components*>...> componentsVectors;
//
//    void OnEntityUpdated(Entity* e) override {
//        // Sprawdzenie, czy encja ma wszystkie wymagane komponenty
//        if ((e->GetComponent<Components>() && ...)) {
//            // unikamy duplikatów
//            auto it = std::find(entities.begin(), entities.end(), e);
//            if (it == entities.end()) {
//                entities.push_back(e);
//                (std::get<std::vector<Components*>>(componentsVectors).push_back(e->GetComponent<Components>()), ...);
//            }
//        }
//        else {
//            // jeśli encja nie pasuje, usuń ją (swap-and-pop)
//            for (size_t i = 0; i < entities.size(); ++i) {
//                if (entities[i] == e) {
//                    entities[i] = entities.back();
//                    entities.pop_back();
//                    ((std::get<std::vector<Components*>>(componentsVectors)[i] =
//                        std::get<std::vector<Components*>>(componentsVectors).back(),
//                        std::get<std::vector<Components*>>(componentsVectors).pop_back()), ...);
//                    break;
//                }
//            }
//        }
//    }
//};
//
//
//class System {
//    /*
//    protected:
//        std::vector<Entity*> registeredEntities;
//        */
//
//public:
//    virtual ~System() {}
//
//    //virtual bool Matches(Entity* e) = 0;
//
//    virtual void Update(ECS& ecs) = 0;
//
//    virtual void OnEntityUpdated(Entity* e) = 0;
//
//    /*
//    void TryRegister(Entity* e) {
//        if (!Matches(e)) return;
//
//        auto it = std::find(registeredEntities.begin(), registeredEntities.end(), e);
//        if (it == registeredEntities.end()) {
//            registeredEntities.push_back(e);
//        }
//    }*/
//
//};
//
//class MovementSystem : public System {
//private:
//    Query<TransformComponent, Rigidbody>* query;
//
//public:
//    MovementSystem(ECS& ecs) {
//        query = ecs.CreateQuery<TransformComponent, Rigidbody>();
//    }
//
//    void OnEntityUpdated(Entity* e) override {
//        query->OnEntityUpdated(e); // forward do query
//    }
//
//    void Update(ECS&) override {
//        auto& transforms = std::get<0>(query->componentsVectors);
//        auto& rigidbodies = std::get<1>(query->componentsVectors);
//
//        for (size_t i = 0; i < query->entities.size(); ++i) {
//            transforms[i]->position.x += rigidbodies[i]->vx;
//            transforms[i]->position.y += rigidbodies[i]->vy;
//
//            // jeśli potrzebujemy, odświeżamy macierz modelu
//            if (transforms[i]->isDirty) {
//                Transform::computeModelMatrix(*transforms[i]);
//            }
//        }
//};
//
//class ECS {
//private:
//    std::vector<std::unique_ptr<Entity>> entities;
//    std::vector<std::unique_ptr<System>> systems;
//    std::vector<std::unique_ptr<QueryBase>> queries;
//
//public:
//    template<typename... Components>
//    Query<Components...>* CreateQuery() {
//        auto* q = new Query<Components...>();
//        for (auto& e : entities)
//            q->OnEntityUpdated(e.get());
//        queries.emplace_back(q);
//        return q;
//    }
//
//    void NotifyEntityChanged(Entity* e) {
//        for (auto& q : queries)
//            q->OnEntityUpdated(e);
//
//        for (auto& sys : systems)
//            sys->OnEntityUpdated(e);
//    }
//
//    void Update() {
//        for (auto& sys : systems)
//            sys->Update(*this);
//    }
//
//    /*
//    template<typename T, typename... Args>
//    T* AddSystemSingleton() {
//        // sprawdzamy, czy system już istnieje
//        for (auto& s : systems) {
//            if (dynamic_cast<T*>(s.get())) return static_cast<T*>(s.get());
//        }
//
//        // tworzymy nowy
//        T* sys = new T(std::forward<Args>(args)...);
//        systems.emplace_back(sys);
//        return sys;
//    }*/
//
//    template<typename T, typename... Args>
//    T* AddSystem(Args&&... args) {
//        T* sys = new T(std::forward<Args>(args)...);
//        systems.emplace_back(sys);
//        for (auto& e : entities)
//            sys->OnEntityUpdated(e.get());
//        return sys;
//    }
//
//    template<typename T>
//    T* GetSystem() {
//        for (auto& sys : systems) {
//            if (auto casted = dynamic_cast<T*>(sys.get()))
//                return casted;
//        }
//        return nullptr;
//    }
//
//    Entity* CreateEntity() {
//        Entity* e = new Entity(this);
//        entities.emplace_back(e);
//        NotifyEntityChanged(e);
//        return e;
//    }
//};
//
//class Scene {
//private:
//    ECS& ecs;  // scena korzysta z ECS, ale go nie posiada
//
//public:
//    Scene(ECS& ecsRef) : ecs(ecsRef) {}
//
//    // Tworzy nową encję, opcjonalnie ustawiając rodzica
//    Entity* CreateEntity(Entity* parent = nullptr) {
//        Entity* e = ecs.CreateEntity();
//
//        // Ustawienie relacji w hierarchii (jeśli encja ma Transform)
//        auto* t = e->GetComponent<Transform>();
//        if (!t) t = e->AddComponent<Transform>();
//
//        if (parent) {
//            auto* pt = parent->GetComponent<Transform>();
//            if (pt) t->parent = pt; // ustawienie rodzica w Transform
//        }
//
//        return e;
//    }
//
//    // Tworzy encję i od razu dodaje komponenty
//    template<typename... Components>
//    Entity* CreateEntityWithComponents(Entity* parent = nullptr) {
//        Entity* e = CreateEntity(parent);
//        (e->AddComponent<Components>(), ...);
//        ecs.NotifyEntityChanged(e);
//        return e;
//    }
//
//    // Tworzenie query w ECS
//    template<typename... Components>
//    Query<Components...>* CreateQuery() {
//        return ecs.CreateQuery<Components...>();
//    }
//
//    void Update(float deltaTime) {
//        ecs.Update();
//    }
//
//    std::vector<Entity*> GetEntities() {
//        return ecs.GetEntities();
//    }
//
//    ECS& GetECS() { return ecs; }
//};
//
//
//int main() {
//    ECS ecs;             // Jeden centralny ECS dla całej gry
//    Scene scene(ecs);    // Scena korzysta z ECS
//
//    // Dodanie systemów globalnie
//    ecs.AddSystem<MovementSystem>();
//    ecs.AddSystem<PhysicsSystem>();
//    ecs.AddSystem<ScriptSystem>();
//
//    // Tworzenie encji przez scenę
//    Entity* player = scene.CreateEntityWithComponents<Transform, Rigidbody>();
//    Entity* enemy = scene.CreateEntityWithComponents<Transform, Rigidbody>();
//
//    // Główna pętla
//    while (true) {
//        float deltaTime = 1.0f / 60.0f;
//
//        ecs.Update(deltaTime);  // Forward update do ECS i systemów
//    }
//
//    return 0;
//}
//
//
///*
//struct Transform : Component {
//    glm::vec3 localPosition{ 0.0f, 0.0f, 0.0f };
//    glm::vec3 localRotation{ 0.0f, 0.0f, 0.0f }; // w stopniach
//    glm::vec3 localScale{ 1.0f, 1.0f, 1.0f };
//
//    glm::mat4 modelMatrix{ 1.0f };
//    bool isDirty{ true };
//}
//
//class TransformSystem : public System {
//private:
//    Query<Transform>* query; // wszystkie encje z Transform
//public:
//    TransformSystem(ECS& ecs) {
//        query = ecs.CreateQuery<Transform>();
//    }
//
//    void OnEntityUpdated(Entity* e) override {
//        query->OnEntityUpdated(e);
//    }
//
//    //void Update(ECS&) override {
//    //    auto& transforms = std::get<0>(query->componentsVectors);
//
//    //    for (size_t i = 0; i < query->entities.size(); ++i) {
//    //        Transform* t = transforms[i];
//
//    //        if (t->parent) {
//    //            Transform* pt = t->parent->GetComponent<Transform>();
//    //            t->modelMatrix = pt->modelMatrix *
//    //                glm::translate(glm::mat4(1.0f), t->localPosition) *
//    //                glm::yawPitchRoll(
//    //                    glm::radians(t->localRotation.y),
//    //                    glm::radians(t->localRotation.x),
//    //                    glm::radians(t->localRotation.z)) *
//    //                glm::scale(glm::mat4(1.0f), t->localScale);
//    //        }
//    //        else {
//    //            t->modelMatrix = glm::translate(glm::mat4(1.0f), t->localPosition) *
//    //                glm::yawPitchRoll(
//    //                    glm::radians(t->localRotation.y),
//    //                    glm::radians(t->localRotation.x),
//    //                    glm::radians(t->localRotation.z)) *
//    //                glm::scale(glm::mat4(1.0f), t->localScale);
//    //        }
//
//    //        t->isDirty = false;
//    //    }
//    //}
//};
//*/
//
//class ScriptSystem : public System {
//private:
//    std::vector<MonoBehaviour*> scripts;
//    bool started = false;
//
//public:
//    void OnEntityUpdated(Entity* e) override {
//        auto comps = e->GetComponents<MonoBehaviour>();
//
//        for (auto* c : comps) {
//            auto it = std::find(scripts.begin(), scripts.end(), c);
//            if (it == scripts.end()) {
//                scripts.push_back(c);
//            }
//        }
//    }
//
//    void Update(ECS& ecs) override {
//        if (!started) {
//            for (auto* s : scripts)
//                s->Start();
//            started = true;
//        }
//
//        for (auto* s : scripts)
//            s->Update(1.0f); // możesz dać deltaTime
//    }
//};
//
//
//
//class MovementSystem : public System {
//private:
//    std::vector<Transform*> transforms;
//    std::vector<Velocity*> velocities;
//
//public:
//    void OnEntityUpdated(Entity* e) override {
//
//        auto* t = e->GetComponent<Transform>();
//        auto* v = e->GetComponent<Velocity>();
//
//        if (t && v) {
//            // unikanie duplikatow
//            auto it = std::find(transforms.begin(), transforms.end(), t);
//            if (it == transforms.end()) {
//                transforms.push_back(t);
//                velocities.push_back(v);
//            }
//        }
//    }
//
//    void Update(ECS& ecs) override {
//        for (size_t i = 0; i < transforms.size(); i++) {
//            transforms[i]->x += velocities[i]->vx;
//            transforms[i]->y += velocities[i]->vy;
//        }
//    }
//};
//
//class PhysicsSystem : public System {
//private:
//    std::vector<Transform*> transforms;
//    std::vector<Rigidbody*> rigidbodies;
//
//public:
//    void OnEntityUpdated(Entity* e) override {
//
//        auto* t = e->GetComponent<Transform>();
//        auto* rb = e->GetComponent<Rigidbody>(); //Przerobic na getComponents
//
//        if (t && rb) {
//            // unikamy duplikatów po Transform*
//            auto it = std::find(transforms.begin(), transforms.end(), t);
//            if (it == transforms.end()) {
//                transforms.push_back(t);
//                rigidbodies.push_back(rb);
//            }
//        }
//    }
//
//
//    void Update(ECS& ecs) override {
//        for (size_t i = 0; i < transforms.size(); i++) {
//            float ay = -9.8f; // grawitacja
//
//            rigidbodies[i]->vy += ay; // aktualizacja prędkości
//            transforms[i]->matrix += rigidbodies[i]->vy; // aktualizacja pozycji
//        }
//    }
//
//    void ApplyForce(Entity* e, float fx, float fy) {
//        auto* rb = e->GetComponent<Rigidbody>();
//        if (rb) {
//            rb->vx += fx;
//            rb->vy += fy;
//        }
//    }
//};
//
//
//
//
//class ECS {
//private:
//    std::vector<std::unique_ptr<Entity>> entities;
//    std::vector<std::unique_ptr<System>> systems;
//
//public:
//    Entity* CreateEntity() {
//        Entity* e = new Entity(entities.size(), this);
//        entities.emplace_back(e);
//
//        RegisterEntityToSystems(e);
//
//        return e;
//    }
//
//    std::vector<Entity*> GetEntities() {
//        std::vector<Entity*> result;
//        for (auto& e : entities)
//            result.push_back(e.get());
//        return result;
//    }
//
//    template<typename T, typename... Args>
//    T* AddSystem(Args&&... args) {
//        T* sys = new T(std::forward<Args>(args)...);
//        systems.emplace_back(sys);
//
//        for (auto& e : entities) {
//            sys->OnEntityUpdated(e.get());
//        }
//
//        return sys;
//    }
//
//    void Update() {
//        for (auto& s : systems)
//            s->Update(*this);
//    }
//
//    /*
//    void RegisterEntityToSystems(Entity* e) {
//        for (auto& sys : systems) {
//            sys->TryRegister(e);
//        }
//    }*/
//
//    void NotifyEntityChanged(Entity* e) {
//        for (auto& sys : systems) {
//            sys->OnEntityUpdated(e);
//        }
//    }
//};
//
//
//int main() {
//    ECS ecs;
//
//    ecs.RegisterComponent<Rigidbody>();
//
//    Entity* e = ecs.CreateEntity();
//
//    ecs.AddComponent<Rigidbody>(e->id, { 0, 0 });
//
//    ecs.AddSystem<MovementSystem>();
//
//    ecs.Update();
//
//    return 0;
//}
//
///*
//class MovementSystem : public System {
//public:
//    bool Matches(Entity* e) override {
//        return e->GetComponent<Transform>() &&
//            e->GetComponent<Velocity>();
//    }
//
//    void Update(ECS& ecs) override {
//        for (auto* e : registeredEntities) {
//            auto* t = e->GetComponent<Transform>();
//            auto* v = e->GetComponent<Velocity>();
//
//            t->x += v->vx;
//            t->y += v->vy;
//        }
//    }
//};
//*/
//
///*
//struct Hierarchy {
//    int parent = -1;                 // -1 = brak rodzica
//    std::vector<int> children;       // lista dzieci
//};
//
//struct Rigidbody {
//    float mass = 0;
//};
//*/
//
///*
//class Entity {
//public:
//    int id;
//    int parent;
//
//    Entity(int id) : id(id) {}
//};
//*/
//
//
///*
//class System {
//public:
//    virtual ~System() {}
//    virtual void Update(std::vector<Entity*>& entities) = 0;
//};*/
//
//
///*
//class MovementSystem : public System {
//public:
//    void Update(ECS& ecs) {
//        for (auto& e : ecs.GetEntities()) {
//
//            auto* t = ecs.GetComponent<Transform>(e->id);
//            auto* v = ecs.GetComponent<Velocity>(e->id);
//
//            if (t && v) {
//                t->x += v->vx;
//                t->y += v->vy;
//            }
//        }
//    }
//};*/
//
///*
//class MovementSystem {
//    std::vector<Entity*> validEntities; // tylko encje z Transform i Velocity
//
//public:
//    void RegisterEntity(Entity* e) {
//        if (e->GetComponent<Transform>() && e->GetComponent<Velocity>()) {
//            validEntities.push_back(e);
//        }
//    }
//
//    void Update() {
//        for (auto* e : validEntities) {
//            auto* t = e->GetComponent<Transform>();
//            auto* v = e->GetComponent<Velocity>();
//            t->x += v->vx;
//            t->y += v->vy;
//        }
//    }
//};
//*/
//
///*
//struct IComponentStorage {
//    virtual ~IComponentStorage() = default;
//};
//
//
//template<typename T>
//class ComponentStorage : public IComponentStorage {
//public:
//    std::unordered_map<int, T> data;
//};
//*/
//
//
//
//
///*
//class ECS {
//private:
//    std::unordered_map<std::type_index, std::unique_ptr<IComponentStorage>> components;
//    std::vector<std::unique_ptr<Entity>> entities;
//    std::vector<std::unique_ptr<System>> systems;
//
//    //std::vector<std::unique_ptr<Entity>> entities;
//    //std::vector<std::unique_ptr<System>> systems;
//    //int nextID = 0;
//
//public:
//
//    Entity* CreateEntity() {
//        Entity* e = new Entity();
//        entities.emplace_back(e);
//        return e;
//    }
//
//    template<typename T>
//    void RegisterComponent() {
//        std::type_index type = typeid(T);
//        components[type] = std::make_unique<ComponentStorage<T>>();
//    }
//
//    template<typename T>
//    void AddComponent(int entityID, const T& component) {
//        auto storage = static_cast<ComponentStorage<T>*>(components[typeid(T)].get());
//        storage->data[entityID] = component;
//    }
//
//    template<typename T>
//    T* GetComponent(int entityID) {
//        auto storage = static_cast<ComponentStorage<T>*>(components[typeid(T)].get());
//
//        auto it = storage->data.find(entityID);
//        if (it != storage->data.end())
//            return &it->second;
//
//        return nullptr;
//    }
//
//    template<typename T>
//    void RemoveComponent(int entityID) {
//        auto storage = static_cast<ComponentStorage<T>*>(components[typeid(T)].get());
//        storage->data.erase(entityID);
//    }
//
//
//    template<typename T, typename... Args>
//    T* AddSystem(Args&&... args) {
//        T* sys = new T(std::forward<Args>(args)...);
//        systems.emplace_back(sys);
//        return sys;
//    }
//
//    void Update() {
//        for (auto& s : systems)
//            s->Update(*this);
//    }
//
//    /*
//    // Dodawanie systemów
//    template<typename T, typename... Args>
//    T* AddSystem(Args&&... args) {
//        T* sys = new T(std::forward<Args>(args)...);
//        systems.emplace_back(sys);
//
//        // zarejestruj wszystkie istniejące encje
//        for (auto& e : entities)
//            sys->RegisterEntity(e.get());
//
//        return sys;
//    }
//
//    // Aktualizacja
//    void Update() {
//        for (auto& s : systems)
//            s->Update();
//    }
//};*/
//
