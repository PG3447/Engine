// ============================================================
//  NavPathSystem - metody benchmarkowe
//
//  Dołącz ten plik do projektu obok istniejącego NavPathSystem.cpp
//  LUB wklej jego zawartość na końcu NavPathSystem.cpp
// ============================================================

#include "NavPathSystem.h"
#include "NavMeshSystem.h"
#include <spdlog/spdlog.h>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <algorithm>
#include <random>
#include <cmath>

// ============================================================
//  Helpers prywatne
// ============================================================

float NavPathSystem::PathLength(const std::vector<glm::vec3>& path) {
    float total = 0.0f;
    for (int i = 0; i + 1 < (int)path.size(); i++) {
        glm::vec3 d = path[i+1] - path[i];
        total += std::sqrt(d.x*d.x + d.z*d.z); // tylko XZ
    }
    return total;
}

float NavPathSystem::PathTurningAngle(const std::vector<glm::vec3>& path) {
    if (path.size() < 3) return 0.0f;

    float totalAngle = 0.0f;
    for (int i = 1; i + 1 < (int)path.size(); i++) {
        glm::vec3 prev = path[i]   - path[i-1];
        glm::vec3 next = path[i+1] - path[i];

        // Normalizuj w XZ
        float lenPrev = std::sqrt(prev.x*prev.x + prev.z*prev.z);
        float lenNext = std::sqrt(next.x*next.x + next.z*next.z);
        if (lenPrev < 1e-6f || lenNext < 1e-6f) continue;

        prev /= lenPrev;
        next /= lenNext;

        // dot product → kąt między kierunkami
        float dot = std::clamp(prev.x*next.x + prev.z*next.z, -1.0f, 1.0f);
        float angle = std::acos(dot) * (180.0f / 3.14159265f); // radiany → stopnie
        totalAngle += angle;
    }
    return totalAngle;
}

float NavPathSystem::EuclideanDistXZ(const glm::vec3& a, const glm::vec3& b) {
    float dx = a.x - b.x;
    float dz = a.z - b.z;
    return std::sqrt(dx*dx + dz*dz);
}

std::vector<glm::vec3> NavPathSystem::TimedPathQuery(
    const glm::vec3& start,
    const glm::vec3& goal,
    const NavMeshData& navData,
    float& outTimeMs)
{
    // Tymczasowy NavPathComponent tylko do przechowania ścieżki
    NavPathComponent tempComp;

    // Ustaw cachedNavMesh_ tymczasowo jeśli nie jest ustawiony
    // (benchmark może być wywołany poza Update loop)
    auto t0 = std::chrono::high_resolution_clock::now();

    bool ok = RequestPath(tempComp, start, goal, navData);

    auto t1 = std::chrono::high_resolution_clock::now();
    outTimeMs = std::chrono::duration<float, std::milli>(t1 - t0).count();

    if (!ok || tempComp.path.empty()) return {};
    return tempComp.path;
}

float NavPathSystem::MeasureCoverage(
    const NavMeshData& navData,
    int numSamples,
    int& outTested,
    int& outWalkable)
{
    if (navData.triangles.empty()) {
        outTested = outWalkable = 0;
        return 0.0f;
    }

    // Wyznacz bounding box NavMesh
    glm::vec3 bbMin( FLT_MAX,  FLT_MAX,  FLT_MAX);
    glm::vec3 bbMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    for (const auto& v : navData.vertices) {
        bbMin = glm::min(bbMin, v.position);
        bbMax = glm::max(bbMax, v.position);
    }

    static std::mt19937 rng(1337);
    std::uniform_real_distribution<float> rx(bbMin.x, bbMax.x);
    std::uniform_real_distribution<float> rz(bbMin.z, bbMax.z);

    outTested   = numSamples;
    outWalkable = 0;

    // Użyj NavMeshComponent::FindTriangle przez navData bezpośrednio
    // (replikujemy logikę FindTriangle żeby nie potrzebować NavMeshComponent*)
    for (int s = 0; s < numSamples; s++) {
        glm::vec3 testPt(rx(rng), bbMin.y, rz(rng));

        for (const auto& tri : navData.triangles) {
            if (!tri.walkable) continue;
            if (tri.v[0] < 0 || tri.v[1] < 0 || tri.v[2] < 0) continue;

            const glm::vec3& vA = navData.vertices[tri.v[0]].position;
            const glm::vec3& vB = navData.vertices[tri.v[1]].position;
            const glm::vec3& vC = navData.vertices[tri.v[2]].position;

            float d1 = (testPt.x - vB.x)*(vA.z - vB.z) - (vA.x - vB.x)*(testPt.z - vB.z);
            float d2 = (testPt.x - vC.x)*(vB.z - vC.z) - (vB.x - vC.x)*(testPt.z - vC.z);
            float d3 = (testPt.x - vA.x)*(vC.z - vA.z) - (vC.x - vA.x)*(testPt.z - vA.z);

            bool hasNeg = (d1 < 0) || (d2 < 0) || (d3 < 0);
            bool hasPos = (d1 > 0) || (d2 > 0) || (d3 > 0);

            if (!(hasNeg && hasPos)) {
                outWalkable++;
                break;
            }
        }
    }

    return (outTested > 0)
        ? (float)outWalkable / (float)outTested * 100.0f
        : 0.0f;
}

// ============================================================
//  RunNavigationBenchmark - główna metoda benchmarkowa
// ============================================================

NavAgentBenchmarkStats NavPathSystem::RunNavigationBenchmark(
    const NavMeshData& navData,
    const std::string& methodName,
    int numQueries,
    int coverageSamples)
{
    NavAgentBenchmarkStats stats;
    stats.methodName   = methodName;
    stats.totalQueries = numQueries;

    spdlog::info("[NavBenchmark] Rozpoczynam benchmark nawigacji: {} ({} zapytań)",
        methodName, numQueries);

    if (navData.triangles.empty()) {
        spdlog::warn("[NavBenchmark] NavMesh jest pusty, pomijam.");
        return stats;
    }

    // Ustaw cachedNavMesh_ na czas benchmarku
    // RequestPath go używa wewnętrznie przez FindTriangle
    // Tworzymy tymczasowy NavMeshComponent na stosie
    NavMeshComponent tempNM;
    tempNM.data = navData; // kopia - benchmark nie modyfikuje danych
    NavMeshComponent* prevCached = cachedNavMesh_;
    cachedNavMesh_ = &tempNM;

    // --- Zbierz walkable trójkąty do losowania punktów ---
    std::vector<int> walkableTris;
    for (int i = 0; i < (int)navData.triangles.size(); i++) {
        if (navData.triangles[i].walkable) walkableTris.push_back(i);
    }

    if (walkableTris.size() < 2) {
        spdlog::warn("[NavBenchmark] Za mało walkable trójkątów do testu.");
        cachedNavMesh_ = prevCached;
        return stats;
    }

    static std::mt19937 rng(42);

    std::vector<float> stretches;
    std::vector<float> astarTimes;
    std::vector<float> pathLengths;
    std::vector<float> waypointCounts;
    std::vector<float> turningAngles;

    stretches.reserve(numQueries);
    astarTimes.reserve(numQueries);

    int successful = 0;

    for (int q = 0; q < numQueries; q++) {
        // Wylosuj dwa różne walkable trójkąty
        std::uniform_int_distribution<int> triDist(0, (int)walkableTris.size() - 1);
        int idxA = triDist(rng);
        int idxB = triDist(rng);
        while (idxB == idxA && walkableTris.size() > 1)
            idxB = triDist(rng);

        // Losowe punkty wewnątrz tych trójkątów (współrzędne barycentryczne)
        auto randomPointInTri = [&](int triIdx) -> glm::vec3 {
            const NavTriangle& tri = navData.triangles[walkableTris[triIdx]];
            const glm::vec3& a = navData.vertices[tri.v[0]].position;
            const glm::vec3& b = navData.vertices[tri.v[1]].position;
            const glm::vec3& c = navData.vertices[tri.v[2]].position;

            std::uniform_real_distribution<float> ud(0.0f, 1.0f);
            float r1 = ud(rng), r2 = ud(rng);
            if (r1 + r2 > 1.0f) { r1 = 1.0f - r1; r2 = 1.0f - r2; }
            float r3 = 1.0f - r1 - r2;
            return r1*a + r2*b + r3*c;
        };

        glm::vec3 startPt = randomPointInTri(idxA);
        glm::vec3 goalPt  = randomPointInTri(idxB);

        float euclidean = EuclideanDistXZ(startPt, goalPt);
        if (euclidean < 0.01f) continue; // start == cel, pomiń

        float timeMs = 0.0f;
        std::vector<glm::vec3> path = TimedPathQuery(startPt, goalPt, navData, timeMs);

        astarTimes.push_back(timeMs);

        if (path.empty()) continue; // A* nie znalazł ścieżki

        successful++;

        float pLen    = PathLength(path);
        float stretch = (euclidean > 0.001f) ? pLen / euclidean : 1.0f;
        float turning = PathTurningAngle(path);

        stretches.push_back(stretch);
        pathLengths.push_back(pLen);
        waypointCounts.push_back((float)path.size());
        turningAngles.push_back(turning);
    }

    // --- Agregacja wyników ---
    stats.successfulQueries = successful;
    stats.successRate = (numQueries > 0)
        ? (float)successful / (float)numQueries
        : 0.0f;

    auto avgVec = [](const std::vector<float>& v) -> float {
        if (v.empty()) return 0.0f;
        return std::accumulate(v.begin(), v.end(), 0.0f) / (float)v.size();
    };
    auto minVec = [](const std::vector<float>& v) -> float {
        if (v.empty()) return 0.0f;
        return *std::min_element(v.begin(), v.end());
    };
    auto maxVec = [](const std::vector<float>& v) -> float {
        if (v.empty()) return 0.0f;
        return *std::max_element(v.begin(), v.end());
    };

    stats.avgPathStretch   = avgVec(stretches);
    stats.minPathStretch   = minVec(stretches);
    stats.maxPathStretch   = maxVec(stretches);
    stats.avgPathLength    = avgVec(pathLengths);
    stats.avgPathWaypoints = avgVec(waypointCounts);
    stats.avgAStarTimeMs   = avgVec(astarTimes);
    stats.minAStarTimeMs   = minVec(astarTimes);
    stats.maxAStarTimeMs   = maxVec(astarTimes);
    stats.avgTurningAngleDeg = avgVec(turningAngles);

    // --- Coverage ---
    stats.coveragePercent = MeasureCoverage(
        navData, coverageSamples,
        stats.coveragePointsTested,
        stats.coveragePointsWalkable);

    cachedNavMesh_ = prevCached;

    spdlog::info("[NavBenchmark] {} — success: {:.1f}%, stretch: {:.3f}, A*: {:.3f}ms, coverage: {:.1f}%",
        methodName,
        stats.successRate * 100.0f,
        stats.avgPathStretch,
        stats.avgAStarTimeMs,
        stats.coveragePercent);

    return stats;
}

// ============================================================
//  Zapis do pliku
// ============================================================

void NavPathSystem::AppendBenchmarkToFile(
    const NavAgentBenchmarkStats& stats,
    const std::string& path)
{
    std::ofstream f(path, std::ios::app);
    if (!f.is_open()) {
        spdlog::error("[NavBenchmark] Nie można otworzyć: {}", path);
        return;
    }

    f << "\n=========================================================\n";
    f << "  NAVIGATION BENCHMARK: " << stats.methodName << "\n";
    f << "=========================================================\n";
    f << "  Skuteczność:\n";
    f << "    Zapytania łącznie    : " << stats.totalQueries << "\n";
    f << "    Znalezione ścieżki   : " << stats.successfulQueries << "\n";
    f << "    Success rate         : " << std::fixed << std::setprecision(1)
      << stats.successRate * 100.0f << "%\n";
    f << "\n  Jakość ścieżki:\n";
    f << "    Path stretch avg     : " << std::setprecision(4) << stats.avgPathStretch
      << "  (ideał = 1.0)\n";
    f << "    Path stretch min/max : " << stats.minPathStretch
      << " / " << stats.maxPathStretch << "\n";
    f << "    Avg długość ścieżki  : " << stats.avgPathLength << "\n";
    f << "    Avg waypointy        : " << std::setprecision(1) << stats.avgPathWaypoints << "\n";
    f << "    Avg kąt zwrotów      : " << stats.avgTurningAngleDeg << " deg\n";
    f << "\n  Czas A*:\n";
    f << "    Avg                  : " << std::setprecision(4) << stats.avgAStarTimeMs << " ms\n";
    f << "    Min / Max            : " << stats.minAStarTimeMs
      << " / " << stats.maxAStarTimeMs << " ms\n";
    f << "\n  Pokrycie przestrzeni:\n";
    f << "    Coverage             : " << std::setprecision(1)
      << stats.coveragePercent << "%\n";
    f << "    Punkty testowe       : " << stats.coveragePointsTested << "\n";
    f << "    Walkable             : " << stats.coveragePointsWalkable << "\n";
}

void NavPathSystem::AppendAllNavigationStatsToFile(
    const std::vector<NavAgentBenchmarkStats>& allStats,
    const std::string& path)
{
    std::ofstream f(path, std::ios::app);
    if (!f.is_open()) return;

    f << "\n\n=========================================================\n";
    f << "  ZESTAWIENIE NAWIGACYJNE — WSZYSTKIE METODY\n";
    f << "=========================================================\n\n";

    // Nagłówek tabeli
    f << std::left
      << std::setw(28) << "Metoda"
      << std::setw(12) << "Success%"
      << std::setw(14) << "Stretch avg"
      << std::setw(14) << "A* avg [ms]"
      << std::setw(12) << "Coverage%"
      << std::setw(14) << "TurningDeg"
      << "\n";
    f << std::string(94, '-') << "\n";

    for (const auto& s : allStats) {
        f << std::left
          << std::setw(28) << s.methodName
          << std::setw(12) << std::fixed << std::setprecision(1) << s.successRate * 100.0f
          << std::setw(14) << std::setprecision(4) << s.avgPathStretch
          << std::setw(14) << s.avgAStarTimeMs
          << std::setw(12) << std::setprecision(1) << s.coveragePercent
          << std::setw(14) << s.avgTurningAngleDeg
          << "\n";
    }

    f << "\n";

    // Rankingi
    auto rank = [&](const std::string& label,
                    std::function<float(const NavAgentBenchmarkStats&)> fn,
                    bool ascending)
    {
        std::vector<const NavAgentBenchmarkStats*> sorted;
        for (const auto& s : allStats) sorted.push_back(&s);
        std::sort(sorted.begin(), sorted.end(),
            [&](const NavAgentBenchmarkStats* a, const NavAgentBenchmarkStats* b) {
                return ascending ? fn(*a) < fn(*b) : fn(*a) > fn(*b);
            });

        f << "  Ranking — " << label << ":\n";
        for (int i = 0; i < (int)sorted.size(); i++) {
            f << "    #" << (i+1) << " " << sorted[i]->methodName
              << " (" << std::fixed << std::setprecision(3) << fn(*sorted[i]) << ")\n";
        }
        f << "\n";
    };

    rank("najniższy path stretch (bliżej 1.0 = lepiej)",
         [](const NavAgentBenchmarkStats& s){ return s.avgPathStretch; }, true);
    rank("najszybszy A*",
         [](const NavAgentBenchmarkStats& s){ return s.avgAStarTimeMs; }, true);
    rank("największe pokrycie",
         [](const NavAgentBenchmarkStats& s){ return s.coveragePercent; }, false);
    rank("najgładsze ścieżki (mniej stopni = lepiej)",
         [](const NavAgentBenchmarkStats& s){ return s.avgTurningAngleDeg; }, true);
}