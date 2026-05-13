#include "NavMeshSystem.h"
#include "core/scene.h"
#include "systems/DebugDrawSystem.h" // DebugDrawSystem::AddLine / AddAABB
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_set>

//  NavMeshComponent - metody

int NavMeshComponent::FindTriangle(const glm::vec3& worldPos) const {
    if (!data.isBaked) return -1;

    for (int i = 0; i < (int)data.triangles.size(); i++) {
        const auto& tri = data.triangles[i];
        if (!tri.walkable) continue;

        const glm::vec3& vA = data.vertices[tri.v[0]].position;
        const glm::vec3& vB = data.vertices[tri.v[1]].position;
        const glm::vec3& vC = data.vertices[tri.v[2]].position;

        // Test w plasczyznie XZ
        float d1 = (worldPos.x - vB.x) * (vA.z - vB.z) - (vA.x - vB.x) * (worldPos.z - vB.z);
        float d2 = (worldPos.x - vC.x) * (vB.z - vC.z) - (vB.x - vC.x) * (worldPos.z - vC.z);
        float d3 = (worldPos.x - vA.x) * (vC.z - vA.z) - (vC.x - vA.x) * (worldPos.z - vA.z);

        bool hasNeg = (d1 < 0) || (d2 < 0) || (d3 < 0);
        bool hasPos = (d1 > 0) || (d2 > 0) || (d3 > 0);

        if (!(hasNeg && hasPos)) return i;
    }
    return -1;
}

bool NavMeshComponent::IsPointWalkable(const glm::vec3& worldPos) const {
    return FindTriangle(worldPos) >= 0;
}

//  NavMeshSystem

NavMeshSystem::NavMeshSystem(ECS& ecs) {
    colliderQuery_ = ecs.CreateQuery<TransformComponent, ColliderComponent>();
}

void NavMeshSystem::OnGameObjectUpdated(GameObject* e) {
    colliderQuery_->OnGameObjectUpdated(e);
}

void NavMeshSystem::Update(ECS& /*ecs*/, float /*dt*/) {
    NavMeshComponent* nm = GetNavMesh();
    if (!nm || !nm->debugDraw || !nm->data.isBaked) return;

    const auto& verts = nm->data.vertices;
    const auto& tris  = nm->data.triangles;

    for (const auto& tri : tris) {
        if (tri.v[0] < 0 || tri.v[1] < 0 || tri.v[2] < 0) continue;

        const glm::vec3& a = verts[tri.v[0]].position;
        const glm::vec3& b = verts[tri.v[1]].position;
        const glm::vec3& c = verts[tri.v[2]].position;

        // Lekko uniesc linie debug aby nie z-fightowaly z podloga
        const float yOff = 0.05f;
        glm::vec3 ao = a + glm::vec3(0, yOff, 0);
        glm::vec3 bo = b + glm::vec3(0, yOff, 0);
        glm::vec3 co = c + glm::vec3(0, yOff, 0);

        glm::vec4 col = tri.walkable ? nm->colorEdge : nm->colorUnwalkable;

        DebugDrawSystem::AddLine(ao, bo, col);
        DebugDrawSystem::AddLine(bo, co, col);
        DebugDrawSystem::AddLine(co, ao, col);
    }
}

NavMeshComponent* NavMeshSystem::GetNavMesh() const {
    if (!navMeshGO_) return nullptr;
    return navMeshGO_->GetComponent<NavMeshComponent>();
}

//  Bake - glowna sciezka

void NavMeshSystem::Bake(Scene& scene) {
    spdlog::info("[NavMesh] Bake start...");

    // Znajdz lub stworz GameObject z NavMeshComponent
    // Szukamy w scenie - jesli juz istnieje, resetujemy
    navMeshGO_ = scene.CreateGameObject(nullptr);
    navMeshGO_->name = "__NavMesh__";
    NavMeshComponent* nm = navMeshGO_->AddComponent<NavMeshComponent>();
    nm->data.Clear();

    // --- Krok 1: Zbierz walkable surfaces ---
    auto surfaces = CollectWalkableSurfaces(scene);
    if (surfaces.empty()) {
        spdlog::warn("[NavMesh] Brak walkable surface'ow - upewnij sie ze podlogi maja ColliderComponent z isWalkable=true");
        return;
    }
    spdlog::info("[NavMesh] Znaleziono {} walkable surface(s)", surfaces.size());

    auto samplePoints = GenerateSamplePoints(surfaces, nm->voxelSize);
    spdlog::info("[NavMesh] Wygenerowano {} punktow probkowania", samplePoints.size());

    if (samplePoints.size() < 3) {
        spdlog::warn("[NavMesh] Za malo punktow do triangulacji (minimum 3)");
        return;
    }

    auto obstacles = CollectObstacles(scene);
    spdlog::info("[NavMesh] Znaleziono {} przeszkod", obstacles.size());

    auto filteredPoints = FilterBlockedPoints(samplePoints, obstacles, nm->agentRadius, nm->agentHeight);
    spdlog::info("[NavMesh] Po filtrowaniu: {} punktow", filteredPoints.size());

    if (filteredPoints.size() < 3) {
        spdlog::warn("[NavMesh] Za malo punktow po filtrowaniu");
        return;
    }

    // --- Krok 4: Triangulacja Delaunay'a ---
    nm->data = BowyerWatson(filteredPoints);

    // --- Krok 5: Sasiedztwo trojkatow ---
    ComputeNeighbors(nm->data);

    nm->data.isBaked = true;

    spdlog::info("[NavMesh] Bake zakończony: {} wierzcholkow, {} trojkatow",
                 nm->data.vertices.size(), nm->data.triangles.size());
}

//  Krok 1: Zbierz walkable surfaces

std::vector<NavMeshSystem::WalkableSurface>
NavMeshSystem::CollectWalkableSurfaces(Scene& scene) {
    std::vector<WalkableSurface> result;

    auto& gos  = colliderQuery_->gameobjects;
    auto& trs  = std::get<0>(colliderQuery_->componentsVectors);
    auto& cols = std::get<1>(colliderQuery_->componentsVectors);

    for (size_t i = 0; i < gos.size(); i++) {
        auto* col = cols[i];
        auto* tr  = trs[i];

        if (!col->isWalkable) continue;

        // Pozycja ze swiata (uzywamy modelMatrix jesli dostepna, inaczej position)
        glm::vec3 worldPos = glm::vec3(tr->modelMatrix[3]);
        glm::vec3 scale    = tr->scale;

        // Rzeczywisty polrozmar w przestrzeni swiata
        glm::vec3 half = col->halfSize * scale;
        glm::vec3 center = worldPos + col->offset;

        WalkableSurface surf;
        surf.min  = center - half;
        surf.max  = center + half;
        surf.yTop = center.y + half.y; // Gorna powierzchnia

        surf.slopeY = 1.0f;

        result.push_back(surf);
    }
    return result;
}

//  Krok 2: Generuj punkty probkowania (siatka XZ)

std::vector<glm::vec3>
NavMeshSystem::GenerateSamplePoints(
    const std::vector<WalkableSurface>& surfaces,
    float voxelSize)
{
    std::vector<glm::vec3> points;

    // Upewnij sie ze voxelSize > 0
    if (voxelSize <= 0.0f) voxelSize = 1.0f;

    for (const auto& surf : surfaces) {
        float xMin = surf.min.x;
        float xMax = surf.max.x;
        float zMin = surf.min.z;
        float zMax = surf.max.z;
        float y    = surf.yTop;

        // Iteruj po siatce XZ z krokiem voxelSize
        for (float x = xMin; x <= xMax + 1e-4f; x += voxelSize) {
            for (float z = zMin; z <= zMax + 1e-4f; z += voxelSize) {
                points.push_back(glm::vec3(
                    std::min(x, xMax),
                    y,
                    std::min(z, zMax)
                ));
            }
        }
    }

    const float eps = voxelSize * 0.1f;
    std::sort(points.begin(), points.end(),
        [eps](const glm::vec3& a, const glm::vec3& b) {
            if (std::abs(a.x - b.x) > eps) return a.x < b.x;
            if (std::abs(a.z - b.z) > eps) return a.z < b.z;
            return a.y < b.y;
        });

    auto last = std::unique(points.begin(), points.end(),
        [eps](const glm::vec3& a, const glm::vec3& b) {
            return std::abs(a.x - b.x) < eps && std::abs(a.z - b.z) < eps;
        });
    points.erase(last, points.end());

    return points;
}

//  Krok 3: Zbierz przeszkody i filtruj punkty

std::vector<NavMeshSystem::Obstacle>
NavMeshSystem::CollectObstacles(Scene& scene) {
    std::vector<Obstacle> result;

    auto& gos  = colliderQuery_->gameobjects;
    auto& trs  = std::get<0>(colliderQuery_->componentsVectors);
    auto& cols = std::get<1>(colliderQuery_->componentsVectors);

    for (size_t i = 0; i < gos.size(); i++) {
        auto* col = cols[i];
        auto* tr  = trs[i];

        // Przeszkoda = affectsNavMesh=true ALE isWalkable=false
        if (!col->affectsNavMesh || col->isWalkable) continue;

        glm::vec3 worldPos = glm::vec3(tr->modelMatrix[3]);
        glm::vec3 scale    = tr->scale;
        glm::vec3 half     = col->halfSize * scale;
        glm::vec3 center   = worldPos + col->offset;

        Obstacle obs;
        obs.min = center - half;
        obs.max = center + half;
        result.push_back(obs);
    }
    return result;
}

std::vector<glm::vec3>
NavMeshSystem::FilterBlockedPoints(
    const std::vector<glm::vec3>& points,
    const std::vector<Obstacle>&  obstacles,
    float agentRadius,
    float agentHeight)
{
    std::vector<glm::vec3> result;
    result.reserve(points.size());

    for (const auto& p : points) {
        bool blocked = false;

        for (const auto& obs : obstacles) {
            // Rozszerz przeszkode o promien agenta
            glm::vec3 expMin = obs.min - glm::vec3(agentRadius, 0.0f, agentRadius);
            glm::vec3 expMax = obs.max + glm::vec3(agentRadius, 0.0f, agentRadius);

            // Sprawdz czy punkt jest wewnatrz rozszerzonej przeszkody (XZ)
            // oraz czy agent (o wysokosci agentHeight) startujacy z p.y miesci sie pionowo
            bool inXZ = p.x >= expMin.x && p.x <= expMax.x &&
                        p.z >= expMin.z && p.z <= expMax.z;

            // Pionowo: przeszkoda jest powyzej punktu i blokuje przejscie
            bool inY  = obs.max.y > p.y && obs.min.y < (p.y + agentHeight);

            if (inXZ && inY) {
                blocked = true;
                break;
            }
        }

        if (!blocked) result.push_back(p);
    }
    return result;
}

//  Krok 4: Triangulacja Bowyer-Watson

NavMeshSystem::Circumcircle
NavMeshSystem::ComputeCircumcircle(
    const Point2D& p0,
    const Point2D& p1,
    const Point2D& p2) const
{
    // Wzor na okrag opisany trojkata
    // Unikamy dzielenia przez zero dla zdegenerowanych trojkatow
    float ax = p1.x - p0.x;
    float ay = p1.z - p0.z;
    float bx = p2.x - p0.x;
    float by = p2.z - p0.z;

    float D = 2.0f * (ax * by - ay * bx);

    Circumcircle cc;
    if (std::abs(D) < 1e-10f) {
        // Zdegenerowany trojkat - zwroc bardzo duzy okrag
        cc.cx = p0.x;
        cc.cz = p0.z;
        cc.r2 = std::numeric_limits<float>::max();
        return cc;
    }

    float ux = (by * (ax * ax + ay * ay) - ay * (bx * bx + by * by)) / D;
    float uy = (ax * (bx * bx + by * by) - bx * (ax * ax + ay * ay)) / D;

    cc.cx = p0.x + ux;
    cc.cz = p0.z + uy;
    cc.r2 = ux * ux + uy * uy;

    return cc;
}

bool NavMeshSystem::InCircumcircle(const Circumcircle& cc, const Point2D& p) const {
    float dx = p.x - cc.cx;
    float dz = p.z - cc.cz;
    return (dx * dx + dz * dz) < (cc.r2 - 1e-10f);
}

void NavMeshSystem::CreateSuperTriangle(
    const std::vector<Point2D>& pts,
    Point2D& sA, Point2D& sB, Point2D& sC) const
{
    // Znajdz bounding box
    float minX = pts[0].x, maxX = pts[0].x;
    float minZ = pts[0].z, maxZ = pts[0].z;

    for (const auto& p : pts) {
        minX = std::min(minX, p.x);
        maxX = std::max(maxX, p.x);
        minZ = std::min(minZ, p.z);
        maxZ = std::max(maxZ, p.z);
    }

    float dx = maxX - minX;
    float dz = maxZ - minZ;
    float delta = std::max(dx, dz) * 10.0f; // 10x zapas

    float midX = (minX + maxX) * 0.5f;
    float midZ = (minZ + maxZ) * 0.5f;

    // Trojkat otaczajacy - duzy trojkat rownoramienny
    sA = { midX - 2.0f * delta,  midZ - delta,       -1 };
    sB = { midX,                  midZ + 2.0f * delta, -1 };
    sC = { midX + 2.0f * delta,  midZ - delta,        -1 };
}

bool NavMeshSystem::TouchesSuperTriangle(const Triangle2D& tri, int superOffset) const {
    return tri.a >= superOffset || tri.b >= superOffset || tri.c >= superOffset;
}

NavMeshData NavMeshSystem::BowyerWatson(const std::vector<glm::vec3>& points3D) {
    NavMeshData result;

    if (points3D.size() < 3) return result;

    // Konwertuj do 2D (XZ)
    std::vector<Point2D> pts;
    pts.reserve(points3D.size());
    for (int i = 0; i < (int)points3D.size(); i++) {
        pts.push_back({ points3D[i].x, points3D[i].z, i });
    }

    // Stworz super-trojkat
    Point2D sA, sB, sC;
    CreateSuperTriangle(pts, sA, sB, sC);

    int superOffset = (int)pts.size();
    pts.push_back(sA);
    pts.push_back(sB);
    pts.push_back(sC);

    // Inicjalizacja triangulacji z super-trojkatem
    std::vector<Triangle2D> triangulation;
    triangulation.reserve(pts.size() * 2);

    Triangle2D superTri(superOffset, superOffset + 1, superOffset + 2);
    superTri.circle = ComputeCircumcircle(pts[superOffset], pts[superOffset + 1], pts[superOffset + 2]);
    triangulation.push_back(superTri);

    // Wstaw kolejno kazdy punkt
    for (int pIdx = 0; pIdx < (int)points3D.size(); pIdx++) {
        const Point2D& point = pts[pIdx];

        // Znajdz wszystkie trojkaty ktorych okrag opisany zawiera punkt
        std::vector<Edge2D> polygon;

        for (auto& tri : triangulation) {
            if (InCircumcircle(tri.circle, point)) {
                tri.bad = true;

                // Dodaj krawedzie tego trojkata do wielokata
                polygon.emplace_back(tri.a, tri.b);
                polygon.emplace_back(tri.b, tri.c);
                polygon.emplace_back(tri.c, tri.a);
            }
        }

        // Usun 'zle' trojkaty
        triangulation.erase(
            std::remove_if(triangulation.begin(), triangulation.end(),
                           [](const Triangle2D& t){ return t.bad; }),
            triangulation.end()
        );

        // Oznacz zduplikowane krawedzie (wspolne krawedzie usuwanych trojkatow)
        for (int i = 0; i < (int)polygon.size(); i++) {
            for (int j = i + 1; j < (int)polygon.size(); j++) {
                if (polygon[i] == polygon[j]) {
                    polygon[i].bad = true;
                    polygon[j].bad = true;
                }
            }
        }

        // Dla kazdej niezawierajacej krawedzi stwórz nowy trojkat
        for (const auto& edge : polygon) {
            if (edge.bad) continue;

            Triangle2D newTri(edge.a, edge.b, pIdx);
            newTri.circle = ComputeCircumcircle(pts[edge.a], pts[edge.b], pts[pIdx]);
            triangulation.push_back(newTri);
        }
    }

    // Usun trojkaty dotykajace super-trojkata
    triangulation.erase(
        std::remove_if(triangulation.begin(), triangulation.end(),
            [&](const Triangle2D& t){
                return TouchesSuperTriangle(t, superOffset);
            }),
        triangulation.end()
    );

    // Konwertuj wynik do NavMeshData
    // Wierzcholki to oryginalne punkty 3D
    result.vertices.resize(points3D.size());
    for (int i = 0; i < (int)points3D.size(); i++) {
        result.vertices[i].position = points3D[i];
    }

    result.triangles.reserve(triangulation.size());
    for (const auto& tri : triangulation) {
        // Sprawdz czy indeksy sa w zakresie (super-trojkat juz usunieto ale ostroznos)
        if (tri.a >= superOffset || tri.b >= superOffset || tri.c >= superOffset) continue;

        NavTriangle navTri(
            pts[tri.a].idx3D,
            pts[tri.b].idx3D,
            pts[tri.c].idx3D
        );

        // Centroid
        const glm::vec3& va = result.vertices[navTri.v[0]].position;
        const glm::vec3& vb = result.vertices[navTri.v[1]].position;
        const glm::vec3& vc = result.vertices[navTri.v[2]].position;
        navTri.centroid = (va + vb + vc) / 3.0f;

        result.triangles.push_back(navTri);
    }

    spdlog::info("[NavMesh] Bowyer-Watson: {} trojkatow z {} punktow",
                 result.triangles.size(), points3D.size());

    return result;
}

//  Krok 5: Sasiedztwo trojkatow

void NavMeshSystem::ComputeNeighbors(NavMeshData& data) {
    // Dla kazdej krawedzi trojkata, znajdz trojkat ktory ja wspoldziela
    // Krawedz trojkata i:
    //   edge 0: v[1] - v[2]  (naprzeciwko v[0])  -> neighbors[0]
    //   edge 1: v[0] - v[2]  (naprzeciwko v[1])  -> neighbors[1]
    //   edge 2: v[0] - v[1]  (naprzeciwko v[2])  -> neighbors[2]

    const int kEdgeA[3] = { 1, 0, 0 }; // Poczatek krawedzi i-tej
    const int kEdgeB[3] = { 2, 2, 1 }; // Koniec krawedzi i-tej

    int n = (int)data.triangles.size();

    for (int i = 0; i < n; i++) {
        for (int ei = 0; ei < 3; ei++) {
            if (data.triangles[i].neighbors[ei] != -1) continue; // Juz znaleziony

            int vA = data.triangles[i].v[kEdgeA[ei]];
            int vB = data.triangles[i].v[kEdgeB[ei]];

            for (int j = i + 1; j < n; j++) {
                for (int ej = 0; ej < 3; ej++) {
                    int wA = data.triangles[j].v[kEdgeA[ej]];
                    int wB = data.triangles[j].v[kEdgeB[ej]];

                    // Krawedzie sa takie same jesli wspoldziela te same dwa wierzcholki
                    bool shared = (vA == wA && vB == wB) || (vA == wB && vB == wA);
                    if (shared) {
                        data.triangles[i].neighbors[ei] = j;
                        data.triangles[j].neighbors[ej] = i;
                        goto nextEdge; // Znalazlismy sasiadujacy trojkat dla tej krawedzi
                    }
                }
            }
            nextEdge:;
        }
    }
}

//  Pomocniczy test punkt w trojkacie 2D

bool NavMeshSystem::PointInTriangle2D(
    float px, float pz,
    float ax, float az,
    float bx, float bz,
    float cx, float cz) const
{
    float d1 = (px - bx) * (az - bz) - (ax - bx) * (pz - bz);
    float d2 = (px - cx) * (bz - cz) - (bx - cx) * (pz - cz);
    float d3 = (px - ax) * (cz - az) - (cx - ax) * (pz - az);

    bool hasNeg = (d1 < 0) || (d2 < 0) || (d3 < 0);
    bool hasPos = (d1 > 0) || (d2 > 0) || (d3 > 0);

    return !(hasNeg && hasPos);
}