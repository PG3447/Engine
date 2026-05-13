//
// Created by kubka on 12.05.2026.
//

#include "NavMeshSystem.h"

void NavMeshSystem::Bake(ECS& ecs) {
    m_triangles.clear();
    std::vector<glm::vec2> points;
    std::vector<std::pair<int, int>> constraintEdges;

    // Przeszukaj ECS pod kątem colliderów
    // Możesz użyć tagów lub masy (static) w RigidbodyComponent, aby odróżnić ściany

    // Przykład: zbieramy wierzchołki podłogi rzutowane na płaszczyznę XZ
    // Dla każdego obiektu o rozmiarze halfSize tworzymy 4 punkty (prostokąt)
    /*
        x-h, z-h ---- x+h, z-h
          |            |
        x-h, z+h ---- x+h, z+h
    */

    // Po zebraniu punktów wywołujemy algorytm triangulacji
    // Triangulate(points, constraintEdges);
}
void NavMeshSystem::DebugDraw() {
    // Zakładając, że masz prosty shader do koloru (np. zielony)
    // shader.use();

    for (const auto& tri : m_triangles) {
        // Rysujemy 3 linie dla każdego trójkąta
        // DrawLine(tri.points[0], tri.points[1]);
        // DrawLine(tri.points[1], tri.points[2]);
        // DrawLine(tri.points[2], tri.points[0]);
    }
}