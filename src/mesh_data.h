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
    glm::vec3 centerLocal = glm::vec3(0.0f);
};

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    AABB aabb;
    std::vector<uint32_t> meshID;

    void setMeshId(int passId, uint32_t id)
    {
        if (passId >= static_cast<int>(meshID.size()))
            meshID.resize(passId + 1, UINT32_MAX);

        meshID[passId] = id;
    }

    uint32_t getMeshId(int passId) const
    {
        if (passId >= static_cast<int>(meshID.size()))
            return UINT32_MAX;

        return meshID[passId];
    }

    bool hasMesh(int passId) const
    {
        return getMeshId(passId) != UINT32_MAX;
    }

    void removeMesh(int passId)
    {
        setMeshId(passId, UINT32_MAX);
    }
};

#endif