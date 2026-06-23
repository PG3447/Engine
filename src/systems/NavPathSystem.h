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

// ============================================================
//  Statystyki skuteczności nawigacyjnej dla jednej metody NavMesh
// ============================================================
struct NavAgentBenchmarkStats {
    std::string methodName;

    // --- Skuteczność ---
    int   totalQueries      = 0;   // Ile zapytań A* wykonano
    int   successfulQueries = 0;   // Ile znalazło ścieżkę
    float successRate       = 0.0f; // successfulQueries / totalQueries [0-1]

    // --- Jakość ścieżki ---
    float avgPathStretch    = 0.0f; // avg(długość_ścieżki / odległość_euklidesowa), ideał = 1.0
    float minPathStretch    = 0.0f;
    float maxPathStretch    = 0.0f;

    float avgPathLength     = 0.0f; // Średnia długość ścieżki [jednostki silnika]
    float avgPathWaypoints  = 0.0f; // Średnia liczba waypointów po Funnel

    // --- Czas A* ---
    float avgAStarTimeMs    = 0.0f; // Średni czas zapytania A* [ms]
    float minAStarTimeMs    = 0.0f;
    float maxAStarTimeMs    = 0.0f;

    // --- Pokrycie przestrzeni ---
    float coveragePercent   = 0.0f; // % punktów testowych które są walkable
    int   coveragePointsTested = 0;
    int   coveragePointsWalkable = 0;

    // --- Smoothness ---
    float avgTurningAngleDeg = 0.0f; // Średnia suma kątów zwrotów na ścieżkę [stopnie]
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

    // ============================================================
    //  Benchmark nawigacyjny
    //
    //  Użycie:
    //    auto stats = navPathSys->RunNavigationBenchmark(
    //        navMesh->data, "Delaunay", 200, 500);
    //    NavPathSystem::AppendBenchmarkToFile(stats, "navmesh_benchmark.txt");
    // ============================================================

    // Uruchom pełny benchmark na podanej NavMeshData
    //   methodName   - nazwa metody (do raportu)
    //   numQueries   - ile losowych par (start, cel) przetestować
    //   coverageSamples - ile punktów użyć do testu pokrycia
    NavAgentBenchmarkStats RunNavigationBenchmark(
        const NavMeshData& navData,
        const std::string& methodName,
        int numQueries      = 200,
        int coverageSamples = 500);

    // Dopisz wyniki do istniejącego pliku benchmark (np. po NavMeshBenchmark)
    static void AppendBenchmarkToFile(
        const NavAgentBenchmarkStats& stats,
        const std::string& path);

    // Wariant dla wielu metod naraz
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

    // Portal = krawedz wspolna dwoch sasiadujacych trojkatow
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

    // ---- Benchmark helpers ----

    // Długość ścieżki (suma odległości między kolejnymi waypointami)
    static float PathLength(const std::vector<glm::vec3>& path);

    // Suma kątów zwrotów na ścieżce [stopnie]
    static float PathTurningAngle(const std::vector<glm::vec3>& path);

    // Odległość euklidesowa w XZ między dwoma punktami
    static float EuclideanDistXZ(const glm::vec3& a, const glm::vec3& b);

    // Pojedyncze zapytanie A* z pomiarem czasu - zwraca ścieżkę i czas [ms]
    std::vector<glm::vec3> TimedPathQuery(
        const glm::vec3& start,
        const glm::vec3& goal,
        const NavMeshData& navData,
        float& outTimeMs);

    // Test pokrycia - jaki % losowych punktów nad surfaces jest walkable
    float MeasureCoverage(
        const NavMeshData& navData,
        int numSamples,
        int& outTested,
        int& outWalkable);

    //  Query
    Query<TransformComponent, NavPathComponent>* agentQuery_ = nullptr;

    NavMeshComponent* cachedNavMesh_ = nullptr;
};


#endif //MIMICRY_EXPERIMENTS_NAVPATHSYSTEM_H
