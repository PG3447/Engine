#include "NavMeshBenchmark.h"
#include "core/scene.h"
//#include <spdlog/spdlog.h>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <queue>
#include <random>
#include <map>

NavMeshStats NavMeshBenchmarkSystem::BakeWithMethod(Scene& scene, NavMeshMethod method) {
    auto t0 = std::chrono::high_resolution_clock::now();

    switch (method) {
        case NavMeshMethod::Delaunay: BakeDelaunay(scene); break;
        case NavMeshMethod::Recast:   BakeRecast(scene);   break;
        case NavMeshMethod::Voronoi:  BakeVoronoi(scene);  break;
        case NavMeshMethod::Grid:     BakeGrid(scene);     break;
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    return ComputeStats(method, ms);
}

void NavMeshBenchmarkSystem::ClearNavMesh(Scene& /*scene*/) {
    NavMeshComponent* nm = GetNavMesh();
    if (nm) {
        nm->data.Clear();
        nm->data.isBaked = false;
    }
    navMeshGO_ = nullptr;
    //spdlog::info("[NavMeshBenchmark] Wyczyszczono dane NavMesh.");
}

void NavMeshBenchmarkSystem::RunFullBenchmark(Scene& scene, const std::string& outputPath) {
    //spdlog::info("[NavMeshBenchmark] Rozpoczynam pełny benchmark...");

    std::vector<NavMeshStats> allStats;

    const std::vector<NavMeshMethod> methods = {
        NavMeshMethod::Delaunay,
        NavMeshMethod::Recast,
        NavMeshMethod::Voronoi,
        NavMeshMethod::Grid,
    };

    for (NavMeshMethod method : methods) {
        //spdlog::info("[NavMeshBenchmark] === Metoda: {} ===", MethodName(method));

        NavMeshStats stats = BakeWithMethod(scene, method);
        allStats.push_back(stats);

        //spdlog::info("[NavMeshBenchmark] Czas: {:.2f} ms | Trójkąty: {} (walk: {}, unwalk: {}) | Pamięć: {} KB",
        //    stats.bakeTimeMs,
        //    stats.totalTriangles,
        //    stats.walkableTriangles,
        //    stats.unwalkableTriangles,
        //    stats.memoryEstimateKB);

        ClearNavMesh(scene);
    }

    SaveStatsToFile(allStats, outputPath);
    //spdlog::info("[NavMeshBenchmark] Wyniki zapisano do: {}", outputPath);
}

void NavMeshBenchmarkSystem::BakeDelaunay(Scene& scene) {
    Bake(scene);
}
void NavMeshBenchmarkSystem::BakeVoronoi(Scene& scene) {
    navMeshGO_ = scene.CreateGameObject(nullptr);
    navMeshGO_->name = "__NavMesh_Voronoi__";
    NavMeshComponent* nm = navMeshGO_->AddComponent<NavMeshComponent>();
    nm->data.Clear();

    auto surfaces  = CollectWalkableSurfaces(scene);
    auto obstacles = CollectObstacles(scene);

    if (surfaces.empty()) {
        //spdlog::warn("[Voronoi] Brak walkable surfaces.");
        return;
    }

    float voronoiVoxelSize = nm->voxelSize * 1.5f;
    auto rawPoints = GenerateSamplePoints(surfaces, voronoiVoxelSize);
    auto filteredPoints = FilterBlockedPoints(rawPoints, obstacles, nm->agentRadius, nm->agentHeight);

    if (filteredPoints.size() < 3) {
        //spdlog::warn("[Voronoi] Za mało punktów.");
        return;
    }

    nm->data = BuildVoronoiNavMesh(filteredPoints, obstacles, nm->agentRadius, nm->agentHeight);
    ComputeNeighbors(nm->data);
    MarkBlockedTriangles(nm->data, obstacles, nm->agentRadius, nm->agentHeight);

    nm->data.isBaked = true;
    //spdlog::info("[Voronoi] Bake zakończony: {} wierzchołków, {} trójkątów",
    //    nm->data.vertices.size(), nm->data.triangles.size());
}

NavMeshData NavMeshBenchmarkSystem::BuildVoronoiNavMesh(
    const std::vector<glm::vec3>& points,
    const std::vector<NavMeshSystem::Obstacle>& obstacles,
    float agentRadius, float agentHeight)
{
    NavMeshData delaunayBase = BowyerWatson(points);

    if (delaunayBase.triangles.empty()) return delaunayBase;

    NavMeshData result;

    std::vector<glm::vec3> voronoiVertices;
    voronoiVertices.reserve(delaunayBase.triangles.size());

    for (const auto& tri : delaunayBase.triangles) {
        const glm::vec3& a = delaunayBase.vertices[tri.v[0]].position;
        const glm::vec3& b = delaunayBase.vertices[tri.v[1]].position;
        const glm::vec3& c = delaunayBase.vertices[tri.v[2]].position;

        float ax = b.x - a.x, az = b.z - a.z;
        float bx = c.x - a.x, bz = c.z - a.z;
        float D = 2.0f * (ax * bz - az * bx);

        glm::vec3 center;
        if (std::abs(D) < 1e-8f) {
            center = tri.centroid;
        } else {
            float ux = (bz * (ax*ax + az*az) - az * (bx*bx + bz*bz)) / D;
            float uz = (ax * (bx*bx + bz*bz) - bx * (ax*ax + az*az)) / D;
            center = glm::vec3(a.x + ux, tri.centroid.y, a.z + uz);
        }

        voronoiVertices.push_back(center);
    }

    std::vector<std::vector<int>> vertexToTriangles(delaunayBase.vertices.size());
    for (int ti = 0; ti < (int)delaunayBase.triangles.size(); ti++) {
        const auto& tri = delaunayBase.triangles[ti];
        for (int vi = 0; vi < 3; vi++) {
            if (tri.v[vi] >= 0 && tri.v[vi] < (int)vertexToTriangles.size())
                vertexToTriangles[tri.v[vi]].push_back(ti);
        }
    }

    result.vertices.resize(delaunayBase.vertices.size() + voronoiVertices.size());
    for (int i = 0; i < (int)delaunayBase.vertices.size(); i++)
        result.vertices[i] = delaunayBase.vertices[i];
    int voronoiOffset = (int)delaunayBase.vertices.size();
    for (int i = 0; i < (int)voronoiVertices.size(); i++) {
        result.vertices[voronoiOffset + i].position = voronoiVertices[i];
    }

    for (int vi = 0; vi < (int)delaunayBase.vertices.size(); vi++) {
        const auto& tris = vertexToTriangles[vi];
        if (tris.size() < 2) continue;

        const glm::vec3& center = delaunayBase.vertices[vi].position;
        std::vector<int> sortedTris = tris;
        std::sort(sortedTris.begin(), sortedTris.end(), [&](int a, int b) {
            const glm::vec3& ca = voronoiVertices[a];
            const glm::vec3& cb = voronoiVertices[b];
            float angA = std::atan2(ca.z - center.z, ca.x - center.x);
            float angB = std::atan2(cb.z - center.z, cb.x - center.x);
            return angA < angB;
        });

        for (int i = 0; i < (int)sortedTris.size(); i++) {
            int t0 = sortedTris[i];
            int t1 = sortedTris[(i + 1) % sortedTris.size()];

            NavTriangle newTri(vi,
                               voronoiOffset + t0,
                               voronoiOffset + t1);

            const glm::vec3& va = result.vertices[vi].position;
            const glm::vec3& vb = result.vertices[voronoiOffset + t0].position;
            const glm::vec3& vc = result.vertices[voronoiOffset + t1].position;
            newTri.centroid = (va + vb + vc) / 3.0f;
            newTri.walkable = true;
            result.triangles.push_back(newTri);
        }
    }

  /*  spdlog::info("[Voronoi] Dual Delaunay: {} wierzchołków, {} trójkątów",
        result.vertices.size(), result.triangles.size());*/

    return result;
}

void NavMeshBenchmarkSystem::BakeGrid(Scene& scene) {
    navMeshGO_ = scene.CreateGameObject(nullptr);
    navMeshGO_->name = "__NavMesh_Grid__";
    NavMeshComponent* nm = navMeshGO_->AddComponent<NavMeshComponent>();
    nm->data.Clear();

    auto surfaces  = CollectWalkableSurfaces(scene);
    auto obstacles = CollectObstacles(scene);

    if (surfaces.empty()) {
        //spdlog::warn("[Grid] Brak walkable surfaces.");
        return;
    }

    float cellSize = nm->voxelSize;
    nm->data = BuildGridNavMesh(surfaces, obstacles, cellSize, nm->agentRadius, nm->agentHeight);
    ComputeNeighbors(nm->data);

    nm->data.isBaked = true;
    //spdlog::info("[Grid] Bake zakończony: {} wierzchołków, {} trójkątów",
    //    nm->data.vertices.size(), nm->data.triangles.size());
}

NavMeshData NavMeshBenchmarkSystem::BuildGridNavMesh(
    const std::vector<NavMeshSystem::WalkableSurface>& surfaces,
    const std::vector<NavMeshSystem::Obstacle>& obstacles,
    float cellSize, float agentRadius, float agentHeight)
{
    NavMeshData result;
    std::map<std::pair<int,int>, int> vertexMap;

    for (const auto& surf : surfaces) {
        int cols = (int)std::ceil((surf.max.x - surf.min.x) / cellSize);
        int rows = (int)std::ceil((surf.max.z - surf.min.z) / cellSize);
        if (cols <= 0 || rows <= 0) continue;

        float y = surf.yTop;

        auto makeKey = [&](int c, int r) -> std::pair<int,int> {
            int globalC = (int)std::round((surf.min.x + c * cellSize) / (cellSize * 0.01f));
            int globalR = (int)std::round((surf.min.z + r * cellSize) / (cellSize * 0.01f));
            return {globalC, globalR};
        };

        auto getVertex = [&](int c, int r) -> int {
            auto key = makeKey(c, r);
            auto it = vertexMap.find(key);
            if (it != vertexMap.end()) return it->second;

            float wx = surf.min.x + c * cellSize;
            float wz = surf.min.z + r * cellSize;
            wx = std::clamp(wx, surf.min.x, surf.max.x);
            wz = std::clamp(wz, surf.min.z, surf.max.z);

            NavVertex v;
            v.position = glm::vec3(wx, y, wz);
            int idx = (int)result.vertices.size();
            result.vertices.push_back(v);
            vertexMap[key] = idx;
            return idx;
        };

        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                float cx = surf.min.x + (c + 0.5f) * cellSize;
                float cz = surf.min.z + (r + 0.5f) * cellSize;
                glm::vec3 cellCenter(cx, y, cz);

                bool blocked = false;
                for (const auto& obs : obstacles) {
                    glm::vec3 expMin = obs.min - glm::vec3(agentRadius, 0.f, agentRadius);
                    glm::vec3 expMax = obs.max + glm::vec3(agentRadius, 0.f, agentRadius);
                    bool inXZ = cx >= expMin.x && cx <= expMax.x &&
                                cz >= expMin.z && cz <= expMax.z;
                    bool inY  = obs.max.y > y && obs.min.y < (y + 2.0f);
                    if (inXZ && inY) { blocked = true; break; }
                }

                int v00 = getVertex(c,   r);
                int v10 = getVertex(c+1, r);
                int v01 = getVertex(c,   r+1);
                int v11 = getVertex(c+1, r+1);

                {
                    NavTriangle tri(v00, v10, v11);
                    const glm::vec3& va = result.vertices[v00].position;
                    const glm::vec3& vb = result.vertices[v10].position;
                    const glm::vec3& vc = result.vertices[v11].position;
                    tri.centroid = (va + vb + vc) / 3.0f;
                    tri.walkable = !blocked;
                    result.triangles.push_back(tri);
                }
                {
                    NavTriangle tri(v00, v11, v01);
                    const glm::vec3& va = result.vertices[v00].position;
                    const glm::vec3& vb = result.vertices[v11].position;
                    const glm::vec3& vc = result.vertices[v01].position;
                    tri.centroid = (va + vb + vc) / 3.0f;
                    tri.walkable = !blocked;
                    result.triangles.push_back(tri);
                }
            }
        }
    }

    //spdlog::info("[Grid] Siatka: {} wierzchołków, {} trójkątów",
    //    result.vertices.size(), result.triangles.size());

    return result;
}

static float TriangleArea2D(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) {
    float abx = b.x - a.x, abz = b.z - a.z;
    float acx = c.x - a.x, acz = c.z - a.z;
    return std::abs(abx * acz - abz * acx) * 0.5f;
}

NavMeshStats NavMeshBenchmarkSystem::ComputeStats(NavMeshMethod method, double bakeTimeMs) const {
    NavMeshStats stats;
    stats.method     = method;
    stats.methodName = MethodName(method);
    stats.bakeTimeMs = bakeTimeMs;

    const NavMeshComponent* nm = GetNavMesh();
    if (!nm || !nm->data.isBaked) return stats;

    const auto& data = nm->data;

    stats.totalVertices   = (int)data.vertices.size();
    stats.totalTriangles  = (int)data.triangles.size();

    size_t vertMem = data.vertices.size()  * sizeof(NavVertex);
    size_t triMem  = data.triangles.size() * sizeof(NavTriangle);
    stats.memoryEstimateKB = (vertMem + triMem) / 1024;

    float totalArea = 0.0f;
    float minArea   = FLT_MAX;
    float maxArea   = -FLT_MAX;

    for (const auto& tri : data.triangles) {
        if (tri.walkable) stats.walkableTriangles++;
        else              stats.unwalkableTriangles++;

        if (tri.v[0] >= 0 && tri.v[1] >= 0 && tri.v[2] >= 0 &&
            tri.v[0] < stats.totalVertices &&
            tri.v[1] < stats.totalVertices &&
            tri.v[2] < stats.totalVertices)
        {
            float area = TriangleArea2D(
                data.vertices[tri.v[0]].position,
                data.vertices[tri.v[1]].position,
                data.vertices[tri.v[2]].position);
            totalArea += area;
            minArea = std::min(minArea, area);
            maxArea = std::max(maxArea, area);
        }
    }

    if (stats.totalTriangles > 0) {
        stats.avgTriangleArea = totalArea / stats.totalTriangles;
        stats.minTriangleArea = (minArea == FLT_MAX) ? 0.0f : minArea;
        stats.maxTriangleArea = (maxArea < 0.0f)     ? 0.0f : maxArea;
    }

    return stats;
}

std::string NavMeshBenchmarkSystem::MethodName(NavMeshMethod m) {
    switch (m) {
        case NavMeshMethod::Delaunay: return "Delaunay (Bowyer-Watson)";
        case NavMeshMethod::Recast:   return "Recast (Voxel Flood-Fill)";
        case NavMeshMethod::Voronoi:  return "Voronoi (Dual Delaunay)";
        case NavMeshMethod::Grid:     return "Grid-Based (Uniform Quads)";
    }
    return "Unknown";
}

void NavMeshBenchmarkSystem::SaveStatsToFile(
    const std::vector<NavMeshStats>& stats,
    const std::string& path)
{
    std::ofstream f(path);
    if (!f.is_open()) {
        //spdlog::error("[NavMeshBenchmark] Nie można otworzyć pliku: {}", path);
        return;
    }

    auto now = std::chrono::system_clock::now();
    auto t   = std::chrono::system_clock::to_time_t(now);

    f << "=========================================================\n";
    f << "  NAVMESH BENCHMARK REPORT\n";
    f << "  Mimicry Experiments Engine\n";
    f << "  Data: " << std::ctime(&t);
    f << "=========================================================\n\n";

    f << std::left
      << std::setw(30) << "Metoda"
      << std::setw(14) << "Czas [ms]"
      << std::setw(14) << "Pamięć [KB]"
      << std::setw(12) << "Wierzchołki"
      << std::setw(12) << "Trójkąty"
      << std::setw(12) << "Walkable"
      << std::setw(14) << "Unwalkable"
      << std::setw(12) << "Avg Area"
      << "\n";
    f << std::string(120, '-') << "\n";

    for (const auto& s : stats) {
        f << std::left
          << std::setw(30) << s.methodName
          << std::setw(14) << std::fixed << std::setprecision(2) << s.bakeTimeMs
          << std::setw(14) << s.memoryEstimateKB
          << std::setw(12) << s.totalVertices
          << std::setw(12) << s.totalTriangles
          << std::setw(12) << s.walkableTriangles
          << std::setw(14) << s.unwalkableTriangles
          << std::setw(12) << std::fixed << std::setprecision(4) << s.avgTriangleArea
          << "\n";
    }

    f << "\n=========================================================\n";
    f << "  SZCZEGÓŁOWE STATYSTYKI\n";
    f << "=========================================================\n\n";

    for (const auto& s : stats) {
        f << "--- " << s.methodName << " ---\n";
        f << "  Czas generowania    : " << std::fixed << std::setprecision(3) << s.bakeTimeMs << " ms\n";
        f << "  Zużycie pamięci     : " << s.memoryEstimateKB << " KB\n";
        f << "  Łączna liczba:\n";
        f << "    Wierzchołki       : " << s.totalVertices << "\n";
        f << "    Trójkąty          : " << s.totalTriangles << "\n";
        f << "    Przechodzalne     : " << s.walkableTriangles << "\n";
        f << "    Nieprzechodzalne  : " << s.unwalkableTriangles << "\n";
        if (s.totalTriangles > 0) {
            float walkPct = 100.0f * s.walkableTriangles / s.totalTriangles;
            f << "    % Przechodzalnych : " << std::fixed << std::setprecision(1) << walkPct << "%\n";
        }
        f << "  Powierzchnia trójkątów:\n";
        f << "    Min               : " << std::fixed << std::setprecision(4) << s.minTriangleArea << "\n";
        f << "    Max               : " << std::fixed << std::setprecision(4) << s.maxTriangleArea << "\n";
        f << "    Średnia           : " << std::fixed << std::setprecision(4) << s.avgTriangleArea << "\n";
        f << "\n";
    }

    f << "=========================================================\n";
    f << "  RANKING (wg czasu generowania)\n";
    f << "=========================================================\n\n";

    std::vector<const NavMeshStats*> sorted;
    for (const auto& s : stats) sorted.push_back(&s);
    std::sort(sorted.begin(), sorted.end(),
        [](const NavMeshStats* a, const NavMeshStats* b) {
            return a->bakeTimeMs < b->bakeTimeMs;
        });

    for (int i = 0; i < (int)sorted.size(); i++) {
        f << "  #" << (i+1) << " " << sorted[i]->methodName
          << " — " << std::fixed << std::setprecision(2) << sorted[i]->bakeTimeMs << " ms\n";
    }

    f << "\n=========================================================\n";
    f << "  RANKING (wg liczby trójkątów walkable)\n";
    f << "=========================================================\n\n";

    std::sort(sorted.begin(), sorted.end(),
        [](const NavMeshStats* a, const NavMeshStats* b) {
            return a->walkableTriangles > b->walkableTriangles;
        });

    for (int i = 0; i < (int)sorted.size(); i++) {
        f << "  #" << (i+1) << " " << sorted[i]->methodName
          << " — " << sorted[i]->walkableTriangles << " trójkątów walkable\n";
    }

    f << "\n=========================================================\n";
    f.close();
}