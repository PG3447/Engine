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


// ============================================================
//  NavMeshSystem
//
//  Uzycie:
//    1. Dodaj NavMeshComponent do dowolnego GameObject (np. root sceny)
//    2. Upewnij sie ze obiekty podlogi maja ColliderComponent z isWalkable=true
//    3. Wywolaj ecs.GetSystem<NavMeshSystem>()->Bake(*scene)
//    4. Debug draw dziala automatycznie jesli navMesh.debugDraw = true
// ============================================================

class NavMeshSystem : public System {
public:
    explicit NavMeshSystem(ECS& ecs);

    void OnGameObjectUpdated(GameObject* e) override;
    void Update(ECS& ecs, float dt) override;

    // Glowna metoda - bake navmesh ze sceny
    void Bake(Scene& scene);

    // Czy bake zostal wykonany
    bool IsBaked() const { return navMeshGO_ != nullptr && GetNavMesh() != nullptr && GetNavMesh()->data.isBaked; }

    // Dostep do danych
    NavMeshComponent* GetNavMesh() const;

private:

    //  Krok 1: Zbierz punkty probkowania z walkable colliderow
    struct WalkableSurface {
        glm::vec3 min;
        glm::vec3 max;
        float     yTop;     // Gorna powierzchnia kolizji (y + halfSize.y)
        float     slopeY;   // Normalna Y (dla plaskich = 1.0)
    };

    std::vector<WalkableSurface> CollectWalkableSurfaces(Scene& scene);

    //  Krok 2: Wygeneruj punkty siatki na powierzchniach
    std::vector<glm::vec3> GenerateSamplePoints(
        const std::vector<WalkableSurface>& surfaces,
        float voxelSize);

    //  Krok 3: Odfiltruj punkty zablokowane przez przeszkody
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

    //  Krok 4: Triangulacja Delaunay'a (Bowyer-Watson 2D w XZ)

    // Punkt 2D dla triangulacji (ignorujemy Y, trzymamy index do 3D)
    struct Point2D {
        float x, z;
        int   idx3D; // Indeks w tablicy punktow 3D
    };

    // Okrag opisany na trojkacie (circumcircle)
    struct Circumcircle {
        float cx, cz, r2; // Srodek i kwadrat promienia
    };

    struct Triangle2D {
        int a, b, c;          // Indeksy w tablicy Point2D
        Circumcircle circle;
        bool bad = false;     // Oznaczony do usuniecia w Bowyer-Watson

        Triangle2D(int a, int b, int c) : a(a), b(b), c(c), bad(false) {}
    };

    struct Edge2D {
        int a, b;
        bool bad = false; // Duplikat - do usuniecia

        Edge2D(int a, int b) : a(a), b(b) {}

        bool operator==(const Edge2D& o) const {
            return (a == o.a && b == o.b) || (a == o.b && b == o.a);
        }
    };

    // Oblicz okrag opisany trojkata
    Circumcircle ComputeCircumcircle(
        const Point2D& p0,
        const Point2D& p1,
        const Point2D& p2) const;

    // Czy punkt lezy wewnatrz okragu opisanego
    bool InCircumcircle(const Circumcircle& cc, const Point2D& p) const;

    // Glowna funkcja triangulacji
    NavMeshData BowyerWatson(const std::vector<glm::vec3>& points3D);

    //  Krok 5: Oblicz sasiedztwo trojkatow
    void ComputeNeighbors(NavMeshData& data);

    // Czy punkt (x, z) jest wewnatrz trojkata 2D (test XZ)
    bool PointInTriangle2D(
        float px, float pz,
        float ax, float az,
        float bx, float bz,
        float cx, float cz) const;

    // Super-trojkat otaczajacy wszystkie punkty
    void CreateSuperTriangle(
        const std::vector<Point2D>& pts,
        Point2D& sA, Point2D& sB, Point2D& sC) const;

    // Czy krawedz (i,j) nalezy do super-trojkata (indeksy >= offset)
    bool TouchesSuperTriangle(const Triangle2D& tri, int superOffset) const;

    GameObject* navMeshGO_ = nullptr;

    // Query do zbierania colliderow ze sceny
    Query<TransformComponent, ColliderComponent>* colliderQuery_ = nullptr;
};

#endif //MIMICRY_EXPERIMENTS_NAVMESHSYSTEM_H
