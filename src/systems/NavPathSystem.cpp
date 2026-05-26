//
// Created by CKLG on 14.05.2026.
//

#include "NavPathSystem.h"
#include "NavMeshSystem.h"
#include "systems/DebugDrawSystem.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cmath>
#include <random>
#include <limits>
NavPathSystem::NavPathSystem(ECS& ecs) {
    agentQuery_ = ecs.CreateQuery<TransformComponent, NavPathComponent>();
}

void NavPathSystem::OnGameObjectUpdated(GameObject* e) {
    agentQuery_->OnGameObjectUpdated(e);
}
void NavPathSystem::Update(ECS& ecs, float dt) {
    // Pobierz navmesh (raz na klatke)
    auto* navSys = ecs.GetSystem<NavMeshSystem>();
    if (!navSys || !navSys->IsBaked()) return;

    cachedNavMesh_ = navSys->GetNavMesh();
    if (!cachedNavMesh_) return;

    const NavMeshData& navData = cachedNavMesh_->data;

    auto& gos   = agentQuery_->gameobjects;
    auto& trs   = std::get<0>(agentQuery_->componentsVectors);
    auto& comps = std::get<1>(agentQuery_->componentsVectors);

    for (size_t i = 0; i < gos.size(); i++) {
        auto* tr   = trs[i];
        auto* comp = comps[i];
        auto* go   = gos[i];

        switch (comp->state) {

        // --- Idle: odczekaj chwile, potem wylosuj nowy cel ---
        case NavAgentState::Idle:
            comp->idleTimer -= dt;
            if (comp->idleTimer <= 0.0f) {
                comp->goalPosition = RandomPointOnNavMesh(navData);
                comp->state = NavAgentState::RequestingPath;
            }
            break;

        // --- RequestingPath: oblicz A* i funnel ---
        case NavAgentState::RequestingPath: {
            glm::vec3 startPos = tr->position;
            bool ok = RequestPath(*comp, startPos, comp->goalPosition, navData);

            if (ok && !comp->path.empty()) {
                comp->currentWaypoint = 0;
                comp->state = NavAgentState::Moving;
            } else {
                // Nie udalo sie - wylosuj nowy cel
                spdlog::warn("[NavPath] Nie znaleziono sciezki, losuje nowy cel");
                comp->goalPosition = RandomPointOnNavMesh(navData);
                // Zostajemy w RequestingPath - sprobuje w nastepnej klatce
            }
            break;
        }

        // --- Moving: przesun agenta wzdluz sciezki ---
        case NavAgentState::Moving:
            MoveAgent(go, *comp, dt);
            break;

        // --- Arrived: dotarl, ustaw idle timer ---
        case NavAgentState::Arrived:
            comp->idleTimer = comp->idleTimeMin +
                ((float)rand() / RAND_MAX) * (comp->idleTimeMax - comp->idleTimeMin);
            comp->path.clear();
            comp->state = NavAgentState::Idle;
            break;
        }

        // --- Debug draw sciezki ---
        if (comp->debugDraw && !comp->path.empty()) {
            const float yOff = 0.1f;
            for (int j = 0; j + 1 < (int)comp->path.size(); j++) {
                glm::vec3 a = comp->path[j]   + glm::vec3(0, yOff, 0);
                glm::vec3 b = comp->path[j+1] + glm::vec3(0, yOff, 0);
                //DebugDrawSystem::AddLine(a, b, comp->colorPath);
            }
            // Aktualny waypoint
            if (comp->currentWaypoint < (int)comp->path.size()) {
                glm::vec3 wp = comp->path[comp->currentWaypoint] + glm::vec3(0, yOff, 0);
                //DebugDrawSystem::AddLine(tr->position + glm::vec3(0, yOff, 0),
                  //                       wp, comp->colorWaypoint);
            }
        }
    }
}

//  RequestPath - A* + Funnel

bool NavPathSystem::RequestPath(NavPathComponent& comp,
                                 const glm::vec3& start,
                                 const glm::vec3& goal,
                                 const NavMeshData& navData)
{
    comp.path.clear();
    comp.currentWaypoint = 0;

    // Znajdz trojkaty startowy i docelowy
    int startTri = cachedNavMesh_->FindTriangle(start);
    int goalTri  = cachedNavMesh_->FindTriangle(goal);

    if (startTri < 0 || goalTri < 0) {
        // Jeden z punktow poza navmeshem - sprobuj najblizszy trojkat
        if (startTri < 0) {
            float bestDist = std::numeric_limits<float>::max();
            for (int i = 0; i < (int)navData.triangles.size(); i++) {
                float d = glm::length(navData.triangles[i].centroid - start);
                if (d < bestDist) { bestDist = d; startTri = i; }
            }
        }
        if (goalTri < 0) {
            float bestDist = std::numeric_limits<float>::max();
            for (int i = 0; i < (int)navData.triangles.size(); i++) {
                float d = glm::length(navData.triangles[i].centroid - goal);
                if (d < bestDist) { bestDist = d; goalTri = i; }
            }
        }
        if (startTri < 0 || goalTri < 0) return false;
    }

    // Jestesmy w tym samym trojkacie - sciezka prosta
    if (startTri == goalTri) {
        comp.path.push_back(start);
        comp.path.push_back(goal);
        return true;
    }

    // A* po trojkatach
    std::vector<int> triPath = AStar(startTri, goalTri, navData);
    if (triPath.empty()) return false;

    // Funnel - wygladzenie sciezki
    comp.path = FunnelPath(triPath, start, goal, navData);

    return !comp.path.empty();
}

//  A* - graf trojkatow

float NavPathSystem::Heuristic(int triA, int triB, const NavMeshData& navData) const {
    // Odleglosc euklidesowa miedzy centroidami w XZ
    glm::vec3 a = navData.triangles[triA].centroid;
    glm::vec3 b = navData.triangles[triB].centroid;
    float dx = a.x - b.x;
    float dz = a.z - b.z;
    return std::sqrt(dx*dx + dz*dz);
}

std::vector<int> NavPathSystem::AStar(int startTri, int goalTri,
                                       const NavMeshData& navData)
{
    int n = (int)navData.triangles.size();

    std::vector<float> g(n, std::numeric_limits<float>::max());
    std::vector<int>   parent(n, -1);
    std::vector<bool>  closed(n, false);

    // Priority queue: min-heap po f
    std::priority_queue<AStarNode,
                        std::vector<AStarNode>,
                        std::greater<AStarNode>> open;

    g[startTri] = 0.0f;
    open.push({ startTri, 0.0f, Heuristic(startTri, goalTri, navData) });

    while (!open.empty()) {
        AStarNode cur = open.top();
        open.pop();

        if (closed[cur.triIndex]) continue;
        closed[cur.triIndex] = true;

        if (cur.triIndex == goalTri) break;

        const NavTriangle& tri = navData.triangles[cur.triIndex];

        for (int ni = 0; ni < 3; ni++) {
            int neighborIdx = tri.neighbors[ni];
            if (neighborIdx < 0 || closed[neighborIdx]) continue;

            const NavTriangle& neighbor = navData.triangles[neighborIdx];
            if (!neighbor.walkable) continue;

            // Koszt = odleglosc miedzy centroidami
            float edgeCost = glm::length(tri.centroid - neighbor.centroid);
            float newG = g[cur.triIndex] + edgeCost;

            if (newG < g[neighborIdx]) {
                g[neighborIdx]      = newG;
                parent[neighborIdx] = cur.triIndex;
                float h = Heuristic(neighborIdx, goalTri, navData);
                open.push({ neighborIdx, newG, newG + h });
            }
        }
    }

    // Odtworz sciezke
    std::vector<int> path;
    if (g[goalTri] == std::numeric_limits<float>::max()) {
        return path; // Nie znaleziono
    }

    for (int cur = goalTri; cur != -1; cur = parent[cur]) {
        path.push_back(cur);
    }
    std::reverse(path.begin(), path.end());

    return path;
}

//  Funnel Algorithm

float NavPathSystem::Cross2D(const glm::vec3& o,
                              const glm::vec3& a,
                              const glm::vec3& b) const
{
    // Cross product w plasczyznie XZ
    return (a.x - o.x) * (b.z - o.z) - (a.z - o.z) * (b.x - o.x);
}

bool NavPathSystem::GetPortal(int fromTri, int toTri,
                               const NavMeshData& navData,
                               glm::vec3& outLeft,
                               glm::vec3& outRight) const
{
    const NavTriangle& from = navData.triangles[fromTri];

    const int kEdgeA[3] = { 1, 0, 0 };
    const int kEdgeB[3] = { 2, 2, 1 };

    for (int e = 0; e < 3; e++) {
        if (from.neighbors[e] == toTri) {
            int iA = from.v[kEdgeA[e]];
            int iB = from.v[kEdgeB[e]];

            glm::vec3 pA = navData.vertices[iA].position;
            glm::vec3 pB = navData.vertices[iB].position;

            // Ustal lewa/prawa strone portalu wzgledem kierunku ruchu
            glm::vec3 dir = navData.triangles[toTri].centroid -
                            navData.triangles[fromTri].centroid;

            // Cross2D(dir, pA-from.centroid) > 0 => pA jest po lewej
            float c = (dir.x) * (pA.z - from.centroid.z) -
                      (dir.z) * (pA.x - from.centroid.x);

            if (c >= 0) {
                outRight  = pA;
                outLeft = pB;
            } else {
                outRight  = pB;
                outLeft = pA;
            }
            return true;
        }
    }
    return false;
}

std::vector<glm::vec3> NavPathSystem::FunnelPath(
    const std::vector<int>& triPath,
    const glm::vec3& startPos,
    const glm::vec3& goalPos,
    const NavMeshData& navData)
{
    std::vector<glm::vec3> result;

    if (triPath.size() == 1) {
        result.push_back(startPos);
        result.push_back(goalPos);
        return result;
    }

    // Zbuduj liste portali
    std::vector<Portal> portals;
    portals.push_back({ startPos, startPos }); // Portal startowy (punkt)

    for (int i = 0; i + 1 < (int)triPath.size(); i++) {
        Portal p;
        if (GetPortal(triPath[i], triPath[i+1], navData, p.left, p.right)) {
            portals.push_back(p);
        }
    }
    portals.push_back({ goalPos, goalPos }); // Portal koncowy (punkt)

    // Simple Stupid Funnel Algorithm
    result.push_back(startPos);

    glm::vec3 apex   = startPos;
    glm::vec3 left   = portals[0].left;
    glm::vec3 right  = portals[0].right;
    int apexIndex    = 0;
    int leftIndex    = 0;
    int rightIndex   = 0;

    for (int i = 1; i < (int)portals.size(); i++) {
        glm::vec3 newLeft  = portals[i].left;
        glm::vec3 newRight = portals[i].right;

        // --- Sprawdz prawa strone ---
        if (Cross2D(apex, right, newRight) <= 0.0f) {
            if (apex == right || Cross2D(apex, left, newRight) > 0.0f) {
                // Zwez lejek z prawej
                right      = newRight;
                rightIndex = i;
            } else {
                // newRight przekroczyl lewa strone - dodaj left jako punkt sciezki
                result.push_back(left);
                apex      = left;
                apexIndex = leftIndex;

                // Zresetuj lejek od nowego apex
                left       = apex;
                right      = apex;
                leftIndex  = apexIndex;
                rightIndex = apexIndex;

                // Cofnij sie do apex i przetworz ponownie
                i = apexIndex;
                continue;
            }
        }

        // --- Sprawdz lewa strone ---
        if (Cross2D(apex, left, newLeft) >= 0.0f) {
            if (apex == left || Cross2D(apex, right, newLeft) < 0.0f) {
                // Zwez lejek z lewej
                left      = newLeft;
                leftIndex = i;
            } else {
                // newLeft przekroczyl prawa strone - dodaj right jako punkt sciezki
                result.push_back(right);
                apex      = right;
                apexIndex = rightIndex;

                // Zresetuj lejek
                left       = apex;
                right      = apex;
                leftIndex  = apexIndex;
                rightIndex = apexIndex;

                i = apexIndex;
                continue;
            }
        }
    }

    // Dodaj cel
    result.push_back(goalPos);

    return result;
}

//  Ruch agenta

void NavPathSystem::MoveAgent(GameObject* go,
                               NavPathComponent& comp,
                               float dt)
{
    if (comp.path.empty() || comp.currentWaypoint >= (int)comp.path.size()) {
        comp.state = NavAgentState::Arrived;
        return;
    }

    auto* tr = go->GetComponent<TransformComponent>();
    if (!tr) return;

    glm::vec3 target = comp.path[comp.currentWaypoint];

    // Ruch tylko w XZ (Y pozostaje jak jest - agent "lezy" na podlodze)
    glm::vec3 toTarget = target - tr->position;
    toTarget.y = 0.0f;

    float dist = glm::length(toTarget);

    // Sprawdz czy dotarl do ostatniego punktu
    bool isLastWaypoint = (comp.currentWaypoint == (int)comp.path.size() - 1);
    float threshold = isLastWaypoint ? comp.arrivalRadius : comp.waypointRadius;

    if (dist < threshold) {
        if (isLastWaypoint) {
            comp.state = NavAgentState::Arrived;
            return;
        }
        comp.currentWaypoint++;
        return;
    }

    // Przesun w kierunku waypointa
    glm::vec3 dir = toTarget / dist; // normalize
    tr->position += dir * comp.moveSpeed * dt;
    tr->isDirty = true;

    // Obrot agenta w kierunku ruchu (opcjonalny - tylko Y)
    if (dist > 0.01f) {
        float angle = std::atan2(-dir.x, -dir.z);
        tr->rotation.y = glm::degrees(angle);
    }
}

//  Losowy punkt na NavMesh

glm::vec3 NavPathSystem::RandomPointOnNavMesh(const NavMeshData& navData) {
    if (navData.triangles.empty()) return glm::vec3(0.0f);

    static std::mt19937 rng(std::random_device{}());

    // Wybierz losowy trojkat (walkable)
    std::vector<int> walkable;
    walkable.reserve(navData.triangles.size());
    for (int i = 0; i < (int)navData.triangles.size(); i++) {
        if (navData.triangles[i].walkable) walkable.push_back(i);
    }
    if (walkable.empty()) return glm::vec3(0.0f);

    std::uniform_int_distribution<int> triDist(0, (int)walkable.size() - 1);
    int triIdx = walkable[triDist(rng)];
    const NavTriangle& tri = navData.triangles[triIdx];

    // Losowy punkt wewnatrz trojkata (metoda barycentryczna)
    std::uniform_real_distribution<float> ud(0.0f, 1.0f);
    float r1 = ud(rng);
    float r2 = ud(rng);

    // Zapewnienie ze punkt jest wewnatrz trojkata
    if (r1 + r2 > 1.0f) {
        r1 = 1.0f - r1;
        r2 = 1.0f - r2;
    }
    float r3 = 1.0f - r1 - r2;

    const glm::vec3& a = navData.vertices[tri.v[0]].position;
    const glm::vec3& b = navData.vertices[tri.v[1]].position;
    const glm::vec3& c = navData.vertices[tri.v[2]].position;

    return r1 * a + r2 * b + r3 * c;
}