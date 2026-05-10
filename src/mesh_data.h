#ifndef MESH_DATA_H
#define MESH_DATA_H

#include <vector>
#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 Position{ 0.0f };
    glm::vec3 Normal{ 0.0f };
    glm::vec2 TexCoords{ 0.0f };
    glm::vec3 Tangent{ 0.0f };
    glm::vec3 Bitangent{ 0.0f };
    int m_BoneIDs[4] = { 0, 0, 0, 0 };
    float m_Weights[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
};

struct AABB {
    glm::vec3 min{ FLT_MAX };
    glm::vec3 max{ -FLT_MAX };
};

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
};

#endif