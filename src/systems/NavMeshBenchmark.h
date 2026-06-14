#pragma once

#include "systems/NavMeshSystem.h"
#include <string>
#include <vector>
#include <chrono>
#include <functional>

//  Uruchamia kilka metod generowania NavMesh-a, mierzy:
//    - Czas generowania ms
//    - Zuzycie pamięci w KB
//    - Liczbe wierzchołków / trójkątów
//    - Liczbe trójkątów walkable / unwalkable
//
//  Po każdym bake-u czyści dane i przechodzi do następnej metody.
//  Wyniki zapisuje do pliku TXT.

enum class NavMeshMethod {
    Delaunay,   // istniejący Delanuay
    Recast,     // Voxel rasteryzacja → region flood-fill → contour → triangulacja
    Voronoi,    // Diagramy Woronoja → dual Delaunay
    Grid,       // Prosta siatka kwadratów (każdy kwadrat → 2 trójkąty)
};

struct NavMeshStats {
    NavMeshMethod method;
    std::string   methodName;

    double  bakeTimeMs      = 0.0;
    size_t  memoryEstimateKB = 0;

    int     totalVertices   = 0;
    int     totalTriangles  = 0;
    int     walkableTriangles   = 0;
    int     unwalkableTriangles = 0;

    // Jakość siatki
    float   avgTriangleArea    = 0.0f;
    float   minTriangleArea    = 0.0f;
    float   maxTriangleArea    = 0.0f;
};

//  Rozszerzenie NavMeshSystem o dodatkowe metody bake
class NavMeshBenchmarkSystem : public NavMeshSystem {
public:
    explicit NavMeshBenchmarkSystem(ECS& ecs) : NavMeshSystem(ecs) {}

    // Bake konkretną metodą i zwróć statystyki
    NavMeshStats BakeWithMethod(Scene& scene, NavMeshMethod method);

    // Wyczyść dane (usuń navMeshGO i zresetuj stan)
    void ClearNavMesh(Scene& scene);

    // Uruchom pełny benchmark wszystkich metod i zapisz do pliku
    void RunFullBenchmark(Scene& scene, const std::string& outputPath);

private:
    //Metoda 1: istniejący Delaunay
    void BakeDelaunay(Scene& scene);

    // Metoda 2: Recast
    void BakeRecast(Scene& scene);

    // Metoda 3: Voronoi Diagram
    void BakeVoronoi(Scene& scene);

    // Metoda 4: Grid-Based
    void BakeGrid(Scene& scene);

    // Helpers
    NavMeshStats ComputeStats(NavMeshMethod method, double bakeTimeMs) const;
    static std::string MethodName(NavMeshMethod m);
    static void SaveStatsToFile(const std::vector<NavMeshStats>& stats,
                                const std::string& path);

    // Recast helpers
    struct VoxelCell {
        bool walkable   = false;
        bool hasObstacle = false;
        int  regionId   = -1;
        float y         = 0.0f;
    };

    struct RecastGrid {
        int cols = 0, rows = 0;
        float voxelSize = 0.0f;
        float originX = 0.0f, originZ = 0.0f;
        std::vector<VoxelCell> cells;

        VoxelCell& At(int c, int r)       { return cells[r * cols + c]; }
        const VoxelCell& At(int c, int r) const { return cells[r * cols + c]; }
        bool Valid(int c, int r) const { return c >= 0 && c < cols && r >= 0 && r < rows; }
    };

    RecastGrid BuildVoxelGrid(
        const std::vector<NavMeshSystem::WalkableSurface>& surfaces,
        const std::vector<NavMeshSystem::Obstacle>& obstacles,
        float voxelSize, float agentRadius, float agentHeight);

    void FloodFillRegions(RecastGrid& grid);

    NavMeshData TriangulateRecastGrid(const RecastGrid& grid);

    // Voronoi helpers
    struct VoronoiSite {
        float x, z;
        int   idx;
    };

    NavMeshData BuildVoronoiNavMesh(
        const std::vector<glm::vec3>& points,
        const std::vector<NavMeshSystem::Obstacle>& obstacles,
        float agentRadius, float agentHeight);

    // Grid helpers
    NavMeshData BuildGridNavMesh(
        const std::vector<NavMeshSystem::WalkableSurface>& surfaces,
        const std::vector<NavMeshSystem::Obstacle>& obstacles,
        float cellSize, float agentRadius, float agentHeight);
};