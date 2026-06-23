#pragma once

#include "systems/NavMeshSystem.h"
#include <string>
#include <vector>
#include <chrono>
#include <functional>

enum class NavMeshMethod {
    Delaunay,
    Recast,
    Voronoi,
    Grid,
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

    float   avgTriangleArea    = 0.0f;
    float   minTriangleArea    = 0.0f;
    float   maxTriangleArea    = 0.0f;
};

class NavMeshBenchmarkSystem : public NavMeshSystem {
public:
    explicit NavMeshBenchmarkSystem(ECS& ecs) : NavMeshSystem(ecs) {}

    NavMeshStats BakeWithMethod(Scene& scene, NavMeshMethod method);

    void ClearNavMesh(Scene& scene);

    void RunFullBenchmark(Scene& scene, const std::string& outputPath);

private:
    void BakeDelaunay(Scene& scene);

    void BakeRecast(Scene& scene);

    void BakeVoronoi(Scene& scene);

    void BakeGrid(Scene& scene);

    NavMeshStats ComputeStats(NavMeshMethod method, double bakeTimeMs) const;
    static std::string MethodName(NavMeshMethod m);
    static void SaveStatsToFile(const std::vector<NavMeshStats>& stats,
                                const std::string& path);

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

    struct VoronoiSite {
        float x, z;
        int   idx;
    };

    NavMeshData BuildVoronoiNavMesh(
        const std::vector<glm::vec3>& points,
        const std::vector<NavMeshSystem::Obstacle>& obstacles,
        float agentRadius, float agentHeight);

    NavMeshData BuildGridNavMesh(
        const std::vector<NavMeshSystem::WalkableSurface>& surfaces,
        const std::vector<NavMeshSystem::Obstacle>& obstacles,
        float cellSize, float agentRadius, float agentHeight);
};