#include "SurfaceDecorationSystem.h"

#include <prefab.h>
#include <core/scene.h>
#include <core/gameobject.h>
#include <core/component.h>
#include <shader.h>

#include <glm/gtc/constants.hpp>
//#include <spdlog/spdlog.h>
//#include <imgui.h>
#include <yaml-cpp/yaml.h>

#include <fstream>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <random>
#include <numeric>

float SurfaceDecorationSystem::RandRange(float lo, float hi)
{
    if (lo >= hi) return lo;
    thread_local std::mt19937 rng{ std::random_device{}() };
    std::uniform_real_distribution<float> dist(lo, hi);
    return dist(rng);
}


std::vector<glm::vec2> SurfaceDecorationSystem::GenerateVoronoiCentroids(
    float originX, float originZ,
    float width,   float depth,
    int   count) const
{
    std::vector<glm::vec2> centroids;
    centroids.reserve(count);

    if (count <= 0) return centroids;

    int gridSide = std::max(1, (int)std::ceil(std::sqrt((float)count)));

    float cellW = width  / (float)gridSide;
    float cellD = depth  / (float)gridSide;

    uint32_t areaSeed =
        ((uint32_t)(std::abs(originX) * 100.0f) * 73856093u) ^
        ((uint32_t)(std::abs(originZ) * 100.0f) * 19349663u) ^
        ((uint32_t)count              * 83492791u);
    std::mt19937 localRng(areaSeed);
    std::uniform_real_distribution<float> jitter(0.2f, 0.8f);

    int generated = 0;
    for (int row = 0; row < gridSide && generated < count; ++row)
    {
        for (int col = 0; col < gridSide && generated < count; ++col)
        {
            float cx = originX + cellW * (col + jitter(localRng));
            float cz = originZ + cellD * (row + jitter(localRng));
            centroids.push_back({ cx, cz });
            ++generated;
        }
    }

    return centroids;
}

float SurfaceDecorationSystem::VoronoiDensityWeight(
    float px, float pz,
    float originX, float originZ,
    float width,   float depth,
    const std::vector<glm::vec2>& centroids,
    float falloff,
    bool  denseCenter) const
{
    if (centroids.empty()) return 1.0f;

    // Znajdź odległość do najbliższego centroidu
    float minDist  = std::numeric_limits<float>::max();
    float maxPossible = std::sqrt(width * width + depth * depth);

    for (const auto& c : centroids)
    {
        float dx   = px - c.x;
        float dz   = pz - c.y;
        float dist = std::sqrt(dx * dx + dz * dz);
        if (dist < minDist) minDist = dist;
    }

    float typicalDist = std::sqrt((width * depth) / (float)centroids.size()) * 0.5f;
    float t = std::min(minDist / (typicalDist * 1.5f), 1.0f);

    float w;
    if (denseCenter)
        w = std::pow(1.0f - t, falloff * 2.0f + 0.5f);   // maleje od centrum
    else
        w = std::pow(t,         falloff * 2.0f + 0.5f);   // rośnie od centrum

    return std::max(w, 0.05f);
}

std::optional<glm::vec2> SurfaceDecorationSystem::SamplePoint(
    float originX, float originZ,
    float width,   float depth,
    float padding,
    float minDistance,
    const std::vector<glm::vec2>& placed,
    const std::vector<glm::vec2>& centroids,
    float falloff,
    bool  denseCenter,
    int   maxAttempts) const
{
    float minX = originX + padding;
    float maxX = originX + width  - padding;
    float minZ = originZ + padding;
    float maxZ = originZ + depth  - padding;

    if (minX >= maxX || minZ >= maxZ) return std::nullopt;

    for (int attempt = 0; attempt < maxAttempts; ++attempt)
    {
        float rx = RandRange(minX, maxX);
        float rz = RandRange(minZ, maxZ);

        float w = VoronoiDensityWeight(
            rx, rz,
            originX, originZ,
            width, depth,
            centroids,
            falloff,
            denseCenter);

        float accept = RandRange(0.0f, 1.0f);
        if (accept > w) continue;

        bool tooClose = false;
        for (const auto& p : placed)
        {
            float dx = rx - p.x;
            float dz = rz - p.y;
            if (dx * dx + dz * dz < minDistance * minDistance)
            {
                tooClose = true;
                break;
            }
        }
        if (tooClose) continue;

        return glm::vec2(rx, rz);
    }

    return std::nullopt;
}

const SurfaceDecorationCandidate* SurfaceDecorationSystem::PickWeighted(
    const std::vector<SurfaceDecorationCandidate>& candidates) const
{
    if (candidates.empty()) return nullptr;

    float total = 0.0f;
    for (const auto& c : candidates) total += std::max(0.0f, c.weight);

    if (total <= 0.0f)
        return &candidates[(int)RandRange(0.0f, (float)candidates.size() - 0.001f)];

    float roll = RandRange(0.0f, total);
    float cumulative = 0.0f;
    for (const auto& c : candidates)
    {
        cumulative += c.weight;
        if (roll <= cumulative) return &c;
    }
    return &candidates.back();
}

SurfaceDecorationConfig& SurfaceDecorationSystem::AddConfig(const SurfaceDecorationConfig& cfg)
{
    m_configs.push_back(cfg);
    return m_configs.back();
}

SurfaceDecorationConfig& SurfaceDecorationSystem::AddConfig(SurfaceDecorationConfig&& cfg)
{
    m_configs.push_back(std::move(cfg));
    return m_configs.back();
}

void SurfaceDecorationSystem::RemoveConfig(int index)
{
    if (index >= 0 && index < (int)m_configs.size())
        m_configs.erase(m_configs.begin() + index);
}

void SurfaceDecorationSystem::ClearConfigs()
{
    m_configs.clear();
}

int SurfaceDecorationSystem::SpawnConfig(
    const SurfaceDecorationConfig& cfg,
    Scene& scene, Shader* shader)
{
    if (!cfg.enabled)             return 0;
    if (cfg.candidates.empty())   return 0;
    if (!cfg.targetObject)
    {
        //spdlog::warn("[SurfaceDecoration] Config '{}': brak targetObject.", cfg.name);
        return 0;
    }

    auto* tr  = cfg.targetObject->GetComponent<TransformComponent>();
    auto* col = cfg.targetObject->GetComponent<ColliderComponent>();

    if (!tr || !col)
    {
        //spdlog::warn("[SurfaceDecoration] Config '{}': targetObject nie ma Transform/Collider.", cfg.name);
        return 0;
    }

    glm::vec3 worldCenter = tr->position + col->offset;
    float surfaceY        = worldCenter.y + col->halfSize.y;

    float originX = worldCenter.x - col->halfSize.x;
    float originZ = worldCenter.z - col->halfSize.z;
    float width   = col->halfSize.x * 2.0f;
    float depth   = col->halfSize.z * 2.0f;

    if (width <= 0.0f || depth <= 0.0f)
    {
        //spdlog::warn("[SurfaceDecoration] Config '{}': collider ma zerowy rozmiar.", cfg.name);
        return 0;
    }

    auto centroids = GenerateVoronoiCentroids(
        originX, originZ, width, depth, cfg.voronoiPoints);

    std::vector<glm::vec2> placed;
    placed.reserve(cfg.totalCount);

    int created = 0;

    for (int i = 0; i < cfg.totalCount; ++i)
    {
        auto point = SamplePoint(
            originX, originZ, width, depth,
            cfg.padding,
            cfg.minDistance,
            placed,
            centroids,
            cfg.densityFalloff,
            cfg.denseCenter);

        if (!point.has_value())
        {
     /*       spdlog::warn("[SurfaceDecoration] Config '{}': nie można umieścić obiektu {} (brak miejsca).",
                cfg.name, i);*/
            continue;
        }

        placed.push_back(*point);

        const SurfaceDecorationCandidate* chosen = PickWeighted(cfg.candidates);
        if (!chosen || !chosen->prefab) continue;

        GameObject* go = chosen->prefab->Instantiate(scene, nullptr, shader);
        if (!go)
        {
            //spdlog::error("[SurfaceDecoration] Instantiate zwrócił null dla '{}'.", chosen->label);
            continue;
        }

        if (auto* objTr = go->GetComponent<TransformComponent>())
        {
            objTr->position = glm::vec3(point->x, surfaceY, point->y)
                            + chosen->localOffset;

            objTr->rotation = glm::vec3(
                RandRange(chosen->rotXMin, chosen->rotXMax),
                RandRange(chosen->rotYMin, chosen->rotYMax),
                RandRange(chosen->rotZMin, chosen->rotZMax));

            float sm      = RandRange(chosen->scaleMin, chosen->scaleMax);
            objTr->scale  = cfg.baseScale * sm;
            objTr->isDirty = true;
        }

        m_spawned.push_back({ go, chosen, chosen->label, cfg.name });
        ++created;
    }

  /*  spdlog::info("[SurfaceDecoration] Config '{}': {} / {} obiektów umieszczonych.",
        cfg.name, created, cfg.totalCount);*/
    return created;
}

int SurfaceDecorationSystem::SpawnAll(Scene& scene, Shader* shader)
{
    int total = 0;
    for (const auto& cfg : m_configs)
        total += SpawnConfig(cfg, scene, shader);

    //spdlog::info("[SurfaceDecoration] SpawnAll(): łącznie {} obiektów.", total);
    return total;
}

void SurfaceDecorationSystem::DespawnAll(Scene& scene)
{
    int count = (int)m_spawned.size();
    for (auto& s : m_spawned)
    {
        if (!s.gameObject) continue;

        if (auto* tr = s.gameObject->GetComponent<TransformComponent>())
        {
            tr->position = glm::vec3(0.0f, -99999.0f, 0.0f);
            tr->isDirty  = true;
        }
        if (auto* col = s.gameObject->GetComponent<ColliderComponent>())
            col->halfSize = glm::vec3(0.0f);
        if (auto* rb = s.gameObject->GetComponent<RigidbodyComponent>())
        {
            rb->useGravity = false;
            rb->isStatic   = true;
            rb->velocity   = glm::vec3(0.0f);
        }
    }
    m_spawned.clear();
    //spdlog::info("[SurfaceDecoration] DespawnAll(): ukryto {} dekoracji.", count);
}
//
//bool SurfaceDecorationSystem::DrawConfigEditor(
//    SurfaceDecorationConfig&                            cfg,
//    const std::vector<std::pair<std::string, Prefab*>>& availablePrefabs,
//    const std::vector<GameObject*>&                     sceneObjects,
//    int                                                 cfgIndex)
//{
//    bool changed = false;
//    ImGui::PushID(cfgIndex);
//
//    changed |= ImGui::InputText("Nazwa##cfgname", cfg.name, sizeof(cfg.name));
//    changed |= ImGui::Checkbox("Aktywna", &cfg.enabled);
//
//    ImGui::Separator();
//
//    ImGui::Text("Obiekt źródłowy (kolider)");
//
//    int curObjIdx = -1;
//    for (int i = 0; i < (int)sceneObjects.size(); ++i)
//        if (sceneObjects[i] == cfg.targetObject) { curObjIdx = i; break; }
//
//    std::vector<const char*> objNames;
//    objNames.reserve(sceneObjects.size());
//    for (auto* go : sceneObjects)
//        objNames.push_back(go ? go->name.c_str() : "(null)");
//
//    if (!objNames.empty())
//    {
//        if (ImGui::Combo("Obiekt##src", &curObjIdx, objNames.data(), (int)objNames.size()))
//        {
//            cfg.targetObject = sceneObjects[curObjIdx];
//            strncpy(cfg.targetName,
//                cfg.targetObject->name.c_str(),
//                sizeof(cfg.targetName) - 1);
//            changed = true;
//        }
//
//        if (cfg.targetObject)
//        {
//            auto* col = cfg.targetObject->GetComponent<ColliderComponent>();
//            auto* tr  = cfg.targetObject->GetComponent<TransformComponent>();
//            if (col && tr)
//            {
//                float surfY  = tr->position.y + col->offset.y + col->halfSize.y;
//                float surfW  = col->halfSize.x * 2.0f;
//                float surfD  = col->halfSize.z * 2.0f;
//                ImGui::TextDisabled("Powierzchnia: %.1f x %.1f  Y=%.2f", surfW, surfD, surfY);
//            }
//            else
//            {
//                ImGui::TextColored(ImVec4(1,0.4f,0.4f,1), "Brak Collider / Transform!");
//            }
//        }
//    }
//    else
//    {
//        ImGui::TextDisabled("(brak obiektów w scenie)");
//    }
//
//    ImGui::Separator();
//
//    ImGui::Text("Generowanie");
//    changed |= ImGui::DragInt  ("Liczba obiektów",   &cfg.totalCount,  1, 0, 2000);
//    changed |= ImGui::DragFloat("Min. dystans",      &cfg.minDistance, 0.01f, 0.0f, 50.0f);
//    changed |= ImGui::DragFloat("Padding",           &cfg.padding,     0.01f, 0.0f, 50.0f);
//
//    ImGui::Separator();
//
//    ImGui::Text("Voronoi density");
//    ImGui::SameLine();
//    ImGui::TextDisabled("(?)");
//    if (ImGui::IsItemHovered())
//        ImGui::SetTooltip(
//            "Voronoi dzieli powierzchnię na komórki.\n"
//            "Obiekty częściej pojawiają się blisko centroidów komórek\n"
//            "(lub daleko — zależnie od trybu).\n"
//            "Falloff kontroluje jak stromo opada gęstość.");
//
//    changed |= ImGui::DragInt("Punkty Voronoi",    &cfg.voronoiPoints,  1, 1, 64);
//    changed |= ImGui::SliderFloat("Falloff gęstości", &cfg.densityFalloff, 0.0f, 1.0f);
//    changed |= ImGui::Checkbox("Gęsto w centrum komórki", &cfg.denseCenter);
//
//    {
//        ImGui::Text("Podgląd gęstości (1D cross-section):");
//        const int BARS = 40;
//        float barVals[BARS];
//        std::vector<glm::vec2> mockCentroids = {{ 0.5f, 0.5f }};
//        for (int b = 0; b < BARS; ++b)
//        {
//            float px = (b + 0.5f) / (float)BARS;
//            barVals[b] = VoronoiDensityWeight(
//                px, 0.5f,
//                0.0f, 0.0f, 1.0f, 1.0f,
//                mockCentroids,
//                cfg.densityFalloff,
//                cfg.denseCenter);
//        }
//        ImGui::PlotHistogram("##density", barVals, BARS, 0,
//            nullptr, 0.0f, 1.0f, ImVec2(0, 40));
//        ImGui::TextDisabled("lewa = brzeg komórki  |  środek = centrum  |  prawa = brzeg");
//    }
//
//    ImGui::Separator();
//
//    ImGui::Text("Bazowa skala");
//    changed |= ImGui::DragFloat3("Skala##bs",
//        reinterpret_cast<float*>(&cfg.baseScale), 0.01f, 0.001f, 100.0f);
//
//    ImGui::Separator();
//
//    ImGui::Text("Kandydaci (%d)", (int)cfg.candidates.size());
//
//    int toRemove = -1;
//    for (int ci = 0; ci < (int)cfg.candidates.size(); ++ci)
//    {
//        ImGui::PushID(ci);
//        auto& c = cfg.candidates[ci];
//
//        ImGui::Separator();
//
//        // Nagłówek kandydata z wagą
//        float totalWeight = 0.0f;
//        for (auto& cc : cfg.candidates) totalWeight += cc.weight;
//        float pct = (totalWeight > 0.0f) ? (c.weight / totalWeight * 100.0f) : 0.0f;
//
//        ImGui::Text("#%d  %s  (%.0f%%)", ci,
//            c.label[0] ? c.label : "(brak prefabu)", pct);
//
//        // Wybór prefabu
//        if (!availablePrefabs.empty())
//        {
//            int curIdx = -1;
//            for (int pi = 0; pi < (int)availablePrefabs.size(); ++pi)
//                if (availablePrefabs[pi].second == c.prefab) { curIdx = pi; break; }
//
//            std::vector<const char*> names;
//            names.reserve(availablePrefabs.size());
//            for (auto& [n, _] : availablePrefabs) names.push_back(n.c_str());
//
//            if (ImGui::Combo("Prefab##p", &curIdx, names.data(), (int)names.size()))
//            {
//                c.prefab = availablePrefabs[curIdx].second;
//                strncpy(c.label,
//                    availablePrefabs[curIdx].first.c_str(),
//                    sizeof(c.label) - 1);
//                changed = true;
//            }
//        }
//        else
//        {
//            ImGui::TextDisabled("(podaj availablePrefabs)");
//        }
//
//        changed |= ImGui::DragFloat("Waga##w",        &c.weight,   0.1f, 0.0f, 1000.0f);
//        changed |= ImGui::DragFloat("Skala min##smin", &c.scaleMin, 0.01f, 0.001f, 100.0f);
//        changed |= ImGui::DragFloat("Skala max##smax", &c.scaleMax, 0.01f, 0.001f, 100.0f);
//
//        if (c.scaleMin > c.scaleMax) c.scaleMin = c.scaleMax;
//
//        // Rotacje — zwinięte żeby nie zajmowały dużo miejsca
//        if (ImGui::TreeNode("Rotacje"))
//        {
//            changed |= ImGui::DragFloat2("Rot Y min/max",
//                reinterpret_cast<float*>(&c.rotYMin), 0.5f, -360.0f, 360.0f);
//            changed |= ImGui::DragFloat2("Rot X min/max",
//                reinterpret_cast<float*>(&c.rotXMin), 0.5f, -180.0f, 180.0f);
//            changed |= ImGui::DragFloat2("Rot Z min/max",
//                reinterpret_cast<float*>(&c.rotZMin), 0.5f, -180.0f, 180.0f);
//            ImGui::TreePop();
//        }
//
//        if (ImGui::TreeNode("Offset lokalny"))
//        {
//            changed |= ImGui::DragFloat3("Offset##off",
//                reinterpret_cast<float*>(&c.localOffset), 0.01f);
//            ImGui::TreePop();
//        }
//
//        if (ImGui::Button("Usuń kandydata##rm")) toRemove = ci;
//
//        ImGui::PopID();
//    }
//
//    if (toRemove >= 0)
//    {
//        cfg.candidates.erase(cfg.candidates.begin() + toRemove);
//        changed = true;
//    }
//
//    if (ImGui::Button("+ Dodaj kandydata"))
//    {
//        cfg.candidates.push_back(SurfaceDecorationCandidate{});
//        changed = true;
//    }
//
//    ImGui::PopID();
//    return changed;
//}
//
//bool SurfaceDecorationSystem::DrawImGui(
//    const std::vector<std::pair<std::string, Prefab*>>& availablePrefabs,
//    const std::vector<GameObject*>&                     sceneObjects,
//    Scene&                                              scene,
//    Shader*                                             shader)
//{
//    bool changed = false;
//
//    ImGui::Begin("Surface Decoration System");
//
//    if (ImGui::Button("Generuj wszystkie"))
//    {
//        DespawnAll(scene);
//        SpawnAll(scene, shader);
//    }
//    ImGui::SameLine();
//    if (ImGui::Button("Usuń dekoracje"))
//        DespawnAll(scene);
//
//    ImGui::SameLine();
//    ImGui::TextDisabled("(%d obiektów)", (int)m_spawned.size());
//
//    ImGui::Separator();
//
//
//    ImGui::InputText("Plik YAML##yaml", m_yamlPath, sizeof(m_yamlPath));
//    if (ImGui::Button("Zapisz##save"))
//    {
//        if (SaveToYaml(m_yamlPath))
//            spdlog::info("[SurfaceDecoration] Zapisano do {}", m_yamlPath);
//        else
//            spdlog::error("[SurfaceDecoration] Błąd zapisu do {}", m_yamlPath);
//    }
//    ImGui::SameLine();
//    if (ImGui::Button("Wczytaj##load"))
//    {
//        if (LoadFromYaml(m_yamlPath, availablePrefabs, sceneObjects))
//        {
//            spdlog::info("[SurfaceDecoration] Wczytano z {}", m_yamlPath);
//            changed = true;
//        }
//        else
//            spdlog::error("[SurfaceDecoration] Błąd wczytu z {}", m_yamlPath);
//    }
//
//    ImGui::Separator();
//    ImGui::TextDisabled("Zapis dokładnego układu (pozycje/rotacje/skale) — do wczytania w runtime:");
//    ImGui::InputText("Plik instancji##instyaml", m_instancesYamlPath, sizeof(m_instancesYamlPath));
//    if (ImGui::Button("Zapisz układ instancji"))
//    {
//        if (SaveInstancesToYaml(m_instancesYamlPath))
//            spdlog::info("[SurfaceDecoration] Zapisano układ instancji do {}", m_instancesYamlPath);
//        else
//            spdlog::error("[SurfaceDecoration] Błąd zapisu instancji do {}", m_instancesYamlPath);
//    }
//
//    ImGui::Separator();
//
//    ImGui::Text("Konfiguracje (%d)", (int)m_configs.size());
//
//    ImGui::BeginChild("##cfglist", ImVec2(200, 300), true);
//    for (int i = 0; i < (int)m_configs.size(); ++i)
//    {
//        ImGui::PushID(i);
//        bool selected = (m_selectedConfig == i);
//
//        // Kolorek jeśli nieaktywna
//        if (!m_configs[i].enabled)
//            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
//
//        if (ImGui::Selectable(m_configs[i].name, selected))
//            m_selectedConfig = i;
//
//        if (!m_configs[i].enabled)
//            ImGui::PopStyleColor();
//
//        ImGui::PopID();
//    }
//    ImGui::EndChild();
//
//    ImGui::SameLine();
//
//    ImGui::BeginGroup();
//
//    if (m_selectedConfig >= 0 && m_selectedConfig < (int)m_configs.size())
//    {
//        ImGui::BeginChild("##cfgedit", ImVec2(0, 300), true);
//        changed |= DrawConfigEditor(
//            m_configs[m_selectedConfig],
//            availablePrefabs,
//            sceneObjects,
//            m_selectedConfig);
//        ImGui::EndChild();
//
//        if (ImGui::Button("Generuj tę konfigurację"))
//        {
//            std::string cfgName = m_configs[m_selectedConfig].name;
//            for (auto it = m_spawned.begin(); it != m_spawned.end(); )
//            {
//                if (it->configName == cfgName)
//                {
//                    if (it->gameObject) scene.DestroyGameObject(it->gameObject);
//                    it = m_spawned.erase(it);
//                }
//                else ++it;
//            }
//            SpawnConfig(m_configs[m_selectedConfig], scene, shader);
//        }
//        ImGui::SameLine();
//        if (ImGui::Button("Usuń konfigurację"))
//        {
//            RemoveConfig(m_selectedConfig);
//            m_selectedConfig = std::min(m_selectedConfig, (int)m_configs.size() - 1);
//            changed = true;
//        }
//    }
//    else
//    {
//        ImGui::BeginChild("##cfgedit", ImVec2(0, 300), true);
//        ImGui::TextDisabled("Wybierz konfigurację z listy po lewej.");
//        ImGui::EndChild();
//    }
//
//    ImGui::EndGroup();
//
//    ImGui::Separator();
//
//    if (ImGui::Button("+ Nowa konfiguracja"))
//    {
//        SurfaceDecorationConfig newCfg;
//        snprintf(newCfg.name, sizeof(newCfg.name), "Config_%d", (int)m_configs.size());
//        AddConfig(std::move(newCfg));
//        m_selectedConfig = (int)m_configs.size() - 1;
//        changed = true;
//    }
//
//    ImGui::End();
//    return changed;
//}

bool SurfaceDecorationSystem::SaveToYaml(const std::string& path) const
{
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "configs" << YAML::Value << YAML::BeginSeq;

    for (const auto& cfg : m_configs)
    {
        out << YAML::BeginMap;
        out << YAML::Key << "name"           << YAML::Value << cfg.name;
        out << YAML::Key << "enabled"        << YAML::Value << cfg.enabled;
        out << YAML::Key << "targetName"     << YAML::Value << cfg.targetName;
        out << YAML::Key << "totalCount"     << YAML::Value << cfg.totalCount;
        out << YAML::Key << "minDistance"    << YAML::Value << cfg.minDistance;
        out << YAML::Key << "padding"        << YAML::Value << cfg.padding;
        out << YAML::Key << "voronoiPoints"  << YAML::Value << cfg.voronoiPoints;
        out << YAML::Key << "densityFalloff" << YAML::Value << cfg.densityFalloff;
        out << YAML::Key << "denseCenter"    << YAML::Value << cfg.denseCenter;
        out << YAML::Key << "baseScaleX"     << YAML::Value << cfg.baseScale.x;
        out << YAML::Key << "baseScaleY"     << YAML::Value << cfg.baseScale.y;
        out << YAML::Key << "baseScaleZ"     << YAML::Value << cfg.baseScale.z;

        out << YAML::Key << "candidates" << YAML::Value << YAML::BeginSeq;
        for (const auto& c : cfg.candidates)
        {
            out << YAML::BeginMap;
            out << YAML::Key << "label"    << YAML::Value << c.label;
            out << YAML::Key << "weight"   << YAML::Value << c.weight;
            out << YAML::Key << "scaleMin" << YAML::Value << c.scaleMin;
            out << YAML::Key << "scaleMax" << YAML::Value << c.scaleMax;
            out << YAML::Key << "rotYMin"  << YAML::Value << c.rotYMin;
            out << YAML::Key << "rotYMax"  << YAML::Value << c.rotYMax;
            out << YAML::Key << "rotXMin"  << YAML::Value << c.rotXMin;
            out << YAML::Key << "rotXMax"  << YAML::Value << c.rotXMax;
            out << YAML::Key << "rotZMin"  << YAML::Value << c.rotZMin;
            out << YAML::Key << "rotZMax"  << YAML::Value << c.rotZMax;
            out << YAML::Key << "offsetX"  << YAML::Value << c.localOffset.x;
            out << YAML::Key << "offsetY"  << YAML::Value << c.localOffset.y;
            out << YAML::Key << "offsetZ"  << YAML::Value << c.localOffset.z;
            out << YAML::EndMap;
        }
        out << YAML::EndSeq; // candidates

        out << YAML::EndMap; // config
    }

    out << YAML::EndSeq; // configs
    out << YAML::EndMap; // root

    std::ofstream fs(path);
    if (!fs.is_open()) return false;
    fs << out.c_str();
    return fs.good();
}

bool SurfaceDecorationSystem::LoadFromYaml(
    const std::string& path,
    const std::vector<std::pair<std::string, Prefab*>>& availablePrefabs,
    const std::vector<GameObject*>&                     sceneObjects)
{
    YAML::Node root;
    try {
        root = YAML::LoadFile(path);
    }
    catch (const YAML::Exception& e) {
        //spdlog::error("[SurfaceDecoration] YAML parse error: {}", e.what());
        return false;
    }

    if (!root["configs"]) return true; // pusty plik — OK

    m_configs.clear();

    auto getF = [](const YAML::Node& n, const char* key, float def) -> float {
        return n[key] ? n[key].as<float>(def) : def;
    };
    auto getI = [](const YAML::Node& n, const char* key, int def) -> int {
        return n[key] ? n[key].as<int>(def) : def;
    };
    auto getB = [](const YAML::Node& n, const char* key, bool def) -> bool {
        return n[key] ? n[key].as<bool>(def) : def;
    };
    auto getS = [](const YAML::Node& n, const char* key) -> std::string {
        return n[key] ? n[key].as<std::string>("") : "";
    };

    for (const auto& cNode : root["configs"])
    {
        SurfaceDecorationConfig cfg;

        auto copyStr = [](const std::string& s, char* buf, size_t len) {
            strncpy(buf, s.c_str(), len - 1);
            buf[len - 1] = '\0';
        };

        copyStr(getS(cNode, "name"),       cfg.name,       sizeof(cfg.name));
        copyStr(getS(cNode, "targetName"), cfg.targetName, sizeof(cfg.targetName));

        cfg.enabled        = getB(cNode, "enabled",        true);
        cfg.totalCount     = getI(cNode, "totalCount",     20);
        cfg.minDistance    = getF(cNode, "minDistance",    0.5f);
        cfg.padding        = getF(cNode, "padding",        0.1f);
        cfg.voronoiPoints  = getI(cNode, "voronoiPoints",  6);
        cfg.densityFalloff = getF(cNode, "densityFalloff", 0.6f);
        cfg.denseCenter    = getB(cNode, "denseCenter",    true);
        cfg.baseScale.x    = getF(cNode, "baseScaleX",     1.0f);
        cfg.baseScale.y    = getF(cNode, "baseScaleY",     1.0f);
        cfg.baseScale.z    = getF(cNode, "baseScaleZ",     1.0f);

        for (auto* go : sceneObjects)
        {
            if (go && go->name == cfg.targetName)
            {
                cfg.targetObject = go;
                break;
            }
        }

        if (cNode["candidates"])
        {
            for (const auto& candNode : cNode["candidates"])
            {
                SurfaceDecorationCandidate c;

                auto label = getS(candNode, "label");
                strncpy(c.label, label.c_str(), sizeof(c.label) - 1);

                c.weight   = getF(candNode, "weight",   1.0f);
                c.scaleMin = getF(candNode, "scaleMin", 1.0f);
                c.scaleMax = getF(candNode, "scaleMax", 1.0f);
                c.rotYMin  = getF(candNode, "rotYMin",  0.0f);
                c.rotYMax  = getF(candNode, "rotYMax",  360.0f);
                c.rotXMin  = getF(candNode, "rotXMin",  0.0f);
                c.rotXMax  = getF(candNode, "rotXMax",  0.0f);
                c.rotZMin  = getF(candNode, "rotZMin",  0.0f);
                c.rotZMax  = getF(candNode, "rotZMax",  0.0f);
                c.localOffset.x = getF(candNode, "offsetX", 0.0f);
                c.localOffset.y = getF(candNode, "offsetY", 0.0f);
                c.localOffset.z = getF(candNode, "offsetZ", 0.0f);

                // Dopasuj prefab po labelu
                for (const auto& [prefabName, prefabPtr] : availablePrefabs)
                {
                    if (prefabName == label)
                    {
                        c.prefab = prefabPtr;
                        break;
                    }
                }

                cfg.candidates.push_back(c);
            }
        }

        m_configs.push_back(std::move(cfg));
    }

    return true;
}

bool SurfaceDecorationSystem::SaveInstancesToYaml(const std::string& path) const
{
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "instances" << YAML::Value << YAML::BeginSeq;

    for (const auto& s : m_spawned)
    {
        if (!s.gameObject || s.prefabLabel.empty()) continue;

        auto* tr = s.gameObject->GetComponent<TransformComponent>();
        if (!tr) continue;

        out << YAML::BeginMap;
        out << YAML::Key << "prefab"     << YAML::Value << s.prefabLabel;
        out << YAML::Key << "config"     << YAML::Value << s.configName;

        out << YAML::Key << "posX" << YAML::Value << tr->position.x;
        out << YAML::Key << "posY" << YAML::Value << tr->position.y;
        out << YAML::Key << "posZ" << YAML::Value << tr->position.z;

        out << YAML::Key << "rotX" << YAML::Value << tr->rotation.x;
        out << YAML::Key << "rotY" << YAML::Value << tr->rotation.y;
        out << YAML::Key << "rotZ" << YAML::Value << tr->rotation.z;

        out << YAML::Key << "scaleX" << YAML::Value << tr->scale.x;
        out << YAML::Key << "scaleY" << YAML::Value << tr->scale.y;
        out << YAML::Key << "scaleZ" << YAML::Value << tr->scale.z;
        out << YAML::EndMap;
    }

    out << YAML::EndSeq;
    out << YAML::EndMap;

    std::ofstream fs(path);
    if (!fs.is_open()) return false;
    fs << out.c_str();

    //spdlog::info("[SurfaceDecoration] Zapisano {} instancji do {}", (int)m_spawned.size(), path);
    return fs.good();
}

int SurfaceDecorationSystem::LoadInstancesFromYaml(
    const std::string& path,
    const std::vector<std::pair<std::string, Prefab*>>& availablePrefabs,
    Scene& scene, Shader* shader)
{
    YAML::Node root;
    try {
        root = YAML::LoadFile(path);
    }
    catch (const YAML::Exception& e) {
        //spdlog::error("[SurfaceDecoration] LoadInstancesFromYaml: {}", e.what());
        return 0;
    }

    if (!root["instances"])
    {
        //spdlog::warn("[SurfaceDecoration] Brak sekcji 'instances' w {}", path);
        return 0;
    }

    auto getF = [](const YAML::Node& n, const char* key, float def) -> float {
        return n[key] ? n[key].as<float>(def) : def;
    };

    int created = 0;

    for (const auto& node : root["instances"])
    {
        std::string prefabLabel = node["prefab"] ? node["prefab"].as<std::string>("") : "";
        std::string configName  = node["config"] ? node["config"].as<std::string>("") : "";

        // Znajdź prefab po nazwie
        Prefab* prefab = nullptr;
        for (const auto& [name, ptr] : availablePrefabs)
        {
            if (name == prefabLabel) { prefab = ptr; break; }
        }

        if (!prefab)
        {
            /*spdlog::warn("[SurfaceDecoration] Nie znaleziono prefabu '{}' przy wczytywaniu instancji.",
                prefabLabel);*/
            continue;
        }

        GameObject* go = prefab->Instantiate(scene, nullptr, shader);
        if (!go) continue;

        if (auto* tr = go->GetComponent<TransformComponent>())
        {
            tr->position = glm::vec3(
                getF(node, "posX", 0.0f),
                getF(node, "posY", 0.0f),
                getF(node, "posZ", 0.0f));

            tr->rotation = glm::vec3(
                getF(node, "rotX", 0.0f),
                getF(node, "rotY", 0.0f),
                getF(node, "rotZ", 0.0f));

            tr->scale = glm::vec3(
                getF(node, "scaleX", 1.0f),
                getF(node, "scaleY", 1.0f),
                getF(node, "scaleZ", 1.0f));

            tr->isDirty = true;
        }

        m_spawned.push_back({ go, nullptr, prefabLabel, configName });
        ++created;
    }

    //spdlog::info("[SurfaceDecoration] Wczytano {} instancji z {}", created, path);
    return created;
}