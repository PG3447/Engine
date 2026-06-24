//
// Created by kubka on 12.05.2026.
//

#ifndef MIMICRY_EXPERIMENTS_NAVMESHSYSTEM_H
#define MIMICRY_EXPERIMENTS_NAVMESHSYSTEM_H

#include <vector>
#include <glm/glm.hpp>
#include "core/component.h"
#include "core/query.h"
#include "core/scene.h"
#include "core/system.h"

class NavMeshSystem : public System {
public:
    explicit NavMeshSystem(ECS& ecs);

    void OnGameObjectUpdated(GameObject* e) override;
    void Update(ECS& ecs, float dt) override;

    void Bake(Scene& scene);

    bool IsBaked() const { return navMeshGO_ != nullptr && GetNavMesh() != nullptr && GetNavMesh()->data.isBaked; }

    NavMeshComponent* GetNavMesh() const;
    struct WalkableSurface {
        glm::vec3 min;
        glm::vec3 max;
        float     yTop;
        float     slopeY;
    };

    std::vector<WalkableSurface> CollectWalkableSurfaces(Scene& scene);

    std::vector<glm::vec3> GenerateSamplePoints(
        const std::vector<WalkableSurface>& surfaces,
        float voxelSize);

    struct Obstacle {
        glm::vec3 min;
        glm::vec3 max;
    };

    std::vector<Obstacle> CollectObstacles(Scene& scene);

    std::vector<glm::vec3> FilterBlockedPoints(
        const std::vector<glm::vec3>& points,
        const std::vector<Obstacle>&  obstacles,
        float agentRadius,
        float agentHeight);

    bool IsPointBlocked(
        const glm::vec3& p,
        const std::vector<Obstacle>& obstacles,
        float agentRadius,
        float agentHeight) const;
    void MarkBlockedTriangles(
        NavMeshData& data,
        const std::vector<Obstacle>& obstacles,
        float agentRadius,
        float agentHeight) const;

    struct Point2D {
        float x, z;
        int   idx3D;
    };

    struct Circumcircle {
        float cx, cz, r2;
    };

    struct Triangle2D {
        int a, b, c;
        Circumcircle circle;
        bool bad = false;

        Triangle2D(int a, int b, int c) : a(a), b(b), c(c), bad(false) {}
    };

    struct Edge2D {
        int a, b;
        bool bad = false;

        Edge2D(int a, int b) : a(a), b(b) {}

        bool operator==(const Edge2D& o) const {
            return (a == o.a && b == o.b) || (a == o.b && b == o.a);
        }
    };

    Circumcircle ComputeCircumcircle(
        const Point2D& p0,
        const Point2D& p1,
        const Point2D& p2) const;

    bool InCircumcircle(const Circumcircle& cc, const Point2D& p) const;

    NavMeshData BowyerWatson(const std::vector<glm::vec3>& points3D);

    void ComputeNeighbors(NavMeshData& data);

    bool PointInTriangle2D(
        float px, float pz,
        float ax, float az,
        float bx, float bz,
        float cx, float cz) const;

    void CreateSuperTriangle(
        const std::vector<Point2D>& pts,
        Point2D& sA, Point2D& sB, Point2D& sC) const;

    bool TouchesSuperTriangle(const Triangle2D& tri, int superOffset) const;

    GameObject* navMeshGO_ = nullptr;

    Query<TransformComponent, ColliderComponent>* colliderQuery_ = nullptr;

    void BakeRecast(Scene& scene);

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
};

#endif //MIMICRY_EXPERIMENTS_NAVMESHSYSTEM_H
