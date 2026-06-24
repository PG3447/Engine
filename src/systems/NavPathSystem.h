//
// Created by CKLG on 14.05.2026.
//

#ifndef MIMICRY_EXPERIMENTS_NAVPATHSYSTEM_H
#define MIMICRY_EXPERIMENTS_NAVPATHSYSTEM_H


#include "core/ecs.h"
#include "core/component.h"
#include <vector>
#include <queue>
#include <unordered_map>
#include <string>

struct NavAgentBenchmarkStats {
    std::string methodName;

    int   totalQueries      = 0;
    int   successfulQueries = 0;
    float successRate       = 0.0f;

    float avgPathStretch    = 0.0f;
    float minPathStretch    = 0.0f;
    float maxPathStretch    = 0.0f;

    float avgPathLength     = 0.0f;
    float avgPathWaypoints  = 0.0f;

    float avgAStarTimeMs    = 0.0f;
    float minAStarTimeMs    = 0.0f;
    float maxAStarTimeMs    = 0.0f;

    float coveragePercent   = 0.0f;
    int   coveragePointsTested = 0;
    int   coveragePointsWalkable = 0;

    float avgTurningAngleDeg = 0.0f;
};

class NavPathSystem : public System {
public:
    explicit NavPathSystem(ECS& ecs);

    void OnGameObjectUpdated(GameObject* e) override;
    void Update(ECS& ecs, float dt) override;

    bool RequestPath(NavPathComponent& comp,
                     const glm::vec3& start,
                     const glm::vec3& goal,
                     const NavMeshData& navData);

    glm::vec3 RandomPointOnNavMesh(const NavMeshData& navData);

    NavAgentBenchmarkStats RunNavigationBenchmark(
        const NavMeshData& navData,
        const std::string& methodName,
        int numQueries      = 200,
        int coverageSamples = 500);

    static void AppendBenchmarkToFile(
        const NavAgentBenchmarkStats& stats,
        const std::string& path);

    static void AppendAllNavigationStatsToFile(
        const std::vector<NavAgentBenchmarkStats>& allStats,
        const std::string& path);

private:
    struct AStarNode {
        int triIndex;
        float g;    // Koszt od startu
        float f;    // g + h

        bool operator>(const AStarNode& o) const { return f > o.f; }
    };

    // Zwraca liste indeksow trojkatow
    std::vector<int> AStar(
        int startTri,
        int goalTri,
        const NavMeshData& navData);

    float Heuristic(int triA, int triB, const NavMeshData& navData) const;

    struct Portal {
        glm::vec3 left;
        glm::vec3 right;
    };

    std::vector<glm::vec3> FunnelPath(
        const std::vector<int>& triPath,
        const glm::vec3& startPos,
        const glm::vec3& goalPos,
        const NavMeshData& navData);

    bool GetPortal(
        int fromTri,
        int toTri,
        const NavMeshData& navData,
        glm::vec3& outLeft,
        glm::vec3& outRight) const;

    float Cross2D(const glm::vec3& o,
                  const glm::vec3& a,
                  const glm::vec3& b) const;

    //  Ruch agenta
    void MoveAgent(GameObject* go,
                   NavPathComponent& comp,
                   float dt);


    static float PathLength(const std::vector<glm::vec3>& path);

    static float PathTurningAngle(const std::vector<glm::vec3>& path);

    static float EuclideanDistXZ(const glm::vec3& a, const glm::vec3& b);

    std::vector<glm::vec3> TimedPathQuery(
        const glm::vec3& start,
        const glm::vec3& goal,
        const NavMeshData& navData,
        float& outTimeMs);

    float MeasureCoverage(
        const NavMeshData& navData,
        int numSamples,
        int& outTested,
        int& outWalkable);

    Query<TransformComponent, NavPathComponent>* agentQuery_ = nullptr;

    NavMeshComponent* cachedNavMesh_ = nullptr;
};


#endif //MIMICRY_EXPERIMENTS_NAVPATHSYSTEM_H
