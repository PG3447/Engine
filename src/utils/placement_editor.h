#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>

#include <glm/glm.hpp>

#include "core/gameobject.h"
#include "core/scene.h"
#include "prefab.h"
#include "yaml_config.h"

namespace PlacementEditor
{
    struct State
    {
        glm::vec3 spawnPosition = glm::vec3(0.0f, 0.0f, 0.0f);

        std::unordered_map<GameObject*, std::string> instanceModelPath;

        std::string yamlPath = "res/Yaml/placements.yaml";

        std::string lastMessage;

        bool spawnWithCollider = false;
    };

    inline State& Get()
    {
        static State state;
        return state;
    }

    inline void RegisterInstance(GameObject* go, const std::string& modelPath)
    {
        if (!go) return;
        Get().instanceModelPath[go] = modelPath;
    }

    inline void UnregisterInstance(GameObject* go)
    {
        Get().instanceModelPath.erase(go);
    }

inline GameObject* SpawnAtCursor(Scene& scene, Prefab& prefab, const std::string& modelPath,
        const std::string& displayName, Shader* shader = nullptr, GameObject* parent = nullptr)
    {
        GameObject* go = prefab.Instantiate(scene, parent, shader);
        if (!go) return nullptr;

        go->name = displayName;

        if (auto* tr = go->GetComponent<TransformComponent>())
        {
            tr->position = Get().spawnPosition;
            tr->rotation = glm::vec3(0.0f);
            tr->scale    = glm::vec3(1.0f);
            tr->isDirty  = true;
        }

        if (Get().spawnWithCollider && !go->GetComponent<ColliderComponent>())
        {
            go->AddComponent<ColliderComponent>();
        }

        RegisterInstance(go, modelPath);
        return go;
    }

    inline bool AppendPlacement(const std::string& modelPath, const std::string& name,
        const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale,
        const ColliderComponent* collider /* nullptr = bez collidera */)
    {
        YamlConfig cfg;

        if (std::filesystem::exists(Get().yamlPath))
        {
            cfg.load(Get().yamlPath);
        }

        YAML::Node root = cfg.getRoot();
        YAML::Node list = root["Placements"];
        if (!list || !list.IsSequence())
        {
            list = YAML::Node(YAML::NodeType::Sequence);
        }

        YAML::Node entry;
        entry["modelPath"] = modelPath;
        entry["name"]      = name;
        entry["position"]  = position;   // korzysta z YAML::convert<glm::vec3> z yaml_config.h
        entry["rotation"]  = rotation;
        entry["scale"]     = scale;

        entry["hasCollider"] = (collider != nullptr);
        if (collider)
        {
            entry["colliderHalfSize"] = collider->halfSize;
            entry["colliderOffset"]   = collider->offset;
            entry["colliderIsTrigger"]    = collider->isTrigger;
            entry["colliderAffectsNavMesh"] = collider->affectsNavMesh;
            entry["colliderIsWalkable"]   = collider->isWalkable;
        }

        list.push_back(entry);
        root["Placements"] = list;

        cfg.getRoot() = root;
        return cfg.save(Get().yamlPath);
    }

    inline bool SaveSelected(GameObject* selected)
    {
        State& st = Get();
        st.lastMessage.clear();

        if (!selected)
        {
            st.lastMessage = "Brak zaznaczonego obiektu.";
            return false;
        }

        auto it = st.instanceModelPath.find(selected);
        if (it == st.instanceModelPath.end())
        {
            st.lastMessage = "Ten obiekt nie zostal zarejestrowany przez PlacementEditor "
                              "(nieznana sciezka modelu) - nie mozna go zapisac.";
            return false;
        }

        auto* tr = selected->GetComponent<TransformComponent>();
        if (!tr)
        {
            st.lastMessage = "Obiekt nie ma TransformComponent.";
            return false;
        }

        const ColliderComponent* col = selected->GetComponent<ColliderComponent>();
        bool ok = AppendPlacement(it->second, selected->name, tr->position, tr->rotation, tr->scale, col);
        st.lastMessage = ok ? ("Zapisano: " + selected->name + (col ? " (z colliderem)" : " (bez collidera)"))
                            : "Blad zapisu do YAML.";
        return ok;
    }

    template <typename PrefabLookupFn>
    inline int LoadPlacements(Scene& scene, PrefabLookupFn prefabLookup, Shader* shader = nullptr)
    {
        if (!std::filesystem::exists(Get().yamlPath))
        {
            //spdlog::info("PlacementEditor: {} nie istnieje jeszcze - nic do wczytania.", Get().yamlPath);
            return 0;
        }

        YamlConfig cfg;
        if (!cfg.load(Get().yamlPath))
        {
            //spdlog::warn("PlacementEditor: nie udalo sie wczytac {}", Get().yamlPath);
            return 0;
        }

        YAML::Node root = cfg.getRoot();
        YAML::Node list = root["Placements"];
        if (!list || !list.IsSequence())
        {
            //spdlog::info("PlacementEditor: plik {} nie zawiera listy 'Placements'.", Get().yamlPath);
            return 0;
        }

        int spawned = 0;
        for (auto node : list)
        {
            if (!node["modelPath"]) continue;

            std::string modelPath = node["modelPath"].as<std::string>();
            std::string name      = node["name"] ? node["name"].as<std::string>() : modelPath;

            Prefab* prefab = prefabLookup(modelPath);
            if (!prefab)
            {
                //spdlog::warn("PlacementEditor: nie znaleziono prefaba dla '{}', pomijam wpis '{}'.", modelPath, name);
                continue;
            }

            GameObject* go = prefab->Instantiate(scene, nullptr, shader);
            if (!go)
            {
                //spdlog::warn("PlacementEditor: Instantiate() zwrocilo nullptr dla '{}'.", modelPath);
                continue;
            }

            go->name = name;

            if (auto* tr = go->GetComponent<TransformComponent>())
            {
                if (node["position"]) tr->position = node["position"].as<glm::vec3>();
                if (node["rotation"]) tr->rotation = node["rotation"].as<glm::vec3>();
                if (node["scale"])    tr->scale    = node["scale"].as<glm::vec3>();
                tr->isDirty = true;
            }

            bool hasCollider = node["hasCollider"] && node["hasCollider"].as<bool>();
            if (hasCollider)
            {
                ColliderComponent* col = go->GetComponent<ColliderComponent>();
                if (!col) col = go->AddComponent<ColliderComponent>();

                if (node["colliderHalfSize"]) col->halfSize = node["colliderHalfSize"].as<glm::vec3>();
                if (node["colliderOffset"])   col->offset   = node["colliderOffset"].as<glm::vec3>();
                if (node["colliderIsTrigger"])        col->isTrigger      = node["colliderIsTrigger"].as<bool>();
                if (node["colliderAffectsNavMesh"])   col->affectsNavMesh = node["colliderAffectsNavMesh"].as<bool>();
                if (node["colliderIsWalkable"])       col->isWalkable     = node["colliderIsWalkable"].as<bool>();

                col->Recalculate(go);
            }

            RegisterInstance(go, modelPath);
            spawned++;
        }

        //spdlog::info("PlacementEditor: wczytano {} obiektow z {}.", spawned, Get().yamlPath);
        return spawned;
    }

    inline Prefab* DefaultPrefabLookup(const std::string& modelPath)
    {
        static std::unordered_map<std::string, Prefab> cache;

        auto modelIt = ResourceManager::Models.find(modelPath);
        if (modelIt == ResourceManager::Models.end() || !modelIt->second)
        {
            //spdlog::warn("PlacementEditor::DefaultPrefabLookup: model '{}' nie jest zaladowany "
            //             "w ResourceManager::Models. Wczytaj go najpierw (LoadModel/connectAllModels).",
            //             modelPath);
            return nullptr;
        }

        auto cacheIt = cache.find(modelPath);
        if (cacheIt == cache.end())
        {
            cacheIt = cache.emplace(modelPath, Prefab(modelIt->second)).first;
        }
        return &cacheIt->second;
    }

   /* inline void DrawImGui(Scene& scene, std::vector<std::pair<std::string, Prefab*>>& availablePrefabs,
        GameObject* selectedGameObject)
    {
        State& st = Get();

        ImGui::Begin("Placement Editor");

        ImGui::Text("Spawn obiektu w punkcie");
        ImGui::DragFloat3("Spawn Position", &st.spawnPosition.x, 0.1f);
        ImGui::Checkbox("Spawn z Colliderem (meble)", &st.spawnWithCollider);
        if (st.spawnWithCollider)
        {
            ImGui::SameLine();
            ImGui::TextDisabled("(rozmiar wyliczany automatycznie z modelu - mozesz poprawic w Collider editorze)");
        }

        static int selectedPrefabIdx = -1;
        static ImGuiTextFilter prefabFilter;

        prefabFilter.Draw("Filtruj prefaby", 200.0f);

        std::vector<int> filteredIndices;
        filteredIndices.reserve(availablePrefabs.size());
        for (int i = 0; i < (int)availablePrefabs.size(); i++)
        {
            if (prefabFilter.PassFilter(availablePrefabs[i].first.c_str()))
                filteredIndices.push_back(i);
        }

        bool selectedStillVisible = false;
        for (int idx : filteredIndices)
            if (idx == selectedPrefabIdx) { selectedStillVisible = true; break; }
        if (!selectedStillVisible)
            selectedPrefabIdx = -1;

        if (ImGui::BeginCombo("Prefab", selectedPrefabIdx >= 0 && selectedPrefabIdx < (int)availablePrefabs.size()
            ? availablePrefabs[selectedPrefabIdx].first.c_str() : "(wybierz)"))
        {
            for (int idx : filteredIndices)
            {
                bool isSelected = (idx == selectedPrefabIdx);
                if (ImGui::Selectable(availablePrefabs[idx].first.c_str(), isSelected))
                    selectedPrefabIdx = idx;
            }
            ImGui::EndCombo();
        }

        bool canSpawn = selectedPrefabIdx >= 0 && selectedPrefabIdx < (int)availablePrefabs.size();
        ImGui::BeginDisabled(!canSpawn);
        if (ImGui::Button("Instantiate at Spawn Point"))
        {
            auto& [path, prefabPtr] = availablePrefabs[selectedPrefabIdx];
            std::string displayName = path;
            size_t slash = displayName.find_last_of("/\\");
            if (slash != std::string::npos) displayName = displayName.substr(slash + 1);
            size_t dot = displayName.find_last_of('.');
            if (dot != std::string::npos) displayName = displayName.substr(0, dot);

            SpawnAtCursor(scene, *prefabPtr, path, displayName);
        }
        ImGui::EndDisabled();

        ImGui::Separator();
        ImGui::Text("Zapis zaznaczonego obiektu do YAML");
        ImGui::Text("Plik: %s", st.yamlPath.c_str());

        if (selectedGameObject)
        {
            bool hasCol = selectedGameObject->GetComponent<ColliderComponent>() != nullptr;
            ImGui::TextColored(hasCol ? ImVec4(0.3f, 1.0f, 0.3f, 1.0f) : ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
                hasCol ? "Ten obiekt ZAPISZE SIE z colliderem." : "Ten obiekt zapisze sie BEZ collidera.");
        }

        ImGui::BeginDisabled(selectedGameObject == nullptr);
        if (ImGui::Button("Zapisz zaznaczony obiekt do placements.yaml"))
        {
            SaveSelected(selectedGameObject);
        }
        ImGui::EndDisabled();

        if (!st.lastMessage.empty())
        {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%s", st.lastMessage.c_str());
        }

        ImGui::End();
    }*/
}