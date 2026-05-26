#ifndef RENDER_MESH_H
#define RENDER_MESH_H

#include <glad/glad.h>
#include "mesh_data.h"

//struct DrawElementsIndirectCommand
//{
//    uint32_t count;          // indicesCount
//    uint32_t instanceCount;  // ile instancji
//    uint32_t firstIndex;     // offset w EBO
//    uint32_t baseVertex;     // offset w VBO
//    uint32_t baseInstance;   // offset w instanceVBO
//};

class RenderMesh {
public:
    unsigned int VAO, VBO, EBO;
    unsigned int indicesCount;
    bool instancingEnabled = false;

    bool instancingPrepared = false;
    unsigned int instanceVBO = 0;

    RenderMesh(const MeshData& data) {
        indicesCount = data.indices.size();
        setupMesh(data);
    }

    ~RenderMesh() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
        if (instanceVBO != 0) {
            glDeleteBuffers(1, &instanceVBO);
        }
    }

    void Draw(GLsizei instanceCount = 0) const {
        glBindVertexArray(VAO);
        if (instanceCount == 0) {
            glDrawElements(GL_TRIANGLES, indicesCount, GL_UNSIGNED_INT, 0);
        }
        else {
            glDrawElementsInstanced(GL_TRIANGLES, indicesCount, GL_UNSIGNED_INT, 0, instanceCount);
        }
        glBindVertexArray(0);
    }

    void EnableInstancing(unsigned int vbo) {
        if (instancingEnabled) return;

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        GLsizei vec4Size = sizeof(glm::vec4);
        glEnableVertexAttribArray(7);
        glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)0);
        glEnableVertexAttribArray(8);
        glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(vec4Size));
        glEnableVertexAttribArray(9);
        glVertexAttribPointer(9, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(2 * vec4Size));
        glEnableVertexAttribArray(10);
        glVertexAttribPointer(10, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(3 * vec4Size));

        glVertexAttribDivisor(7, 1);
        glVertexAttribDivisor(8, 1);
        glVertexAttribDivisor(9, 1);
        glVertexAttribDivisor(10, 1);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        instancingEnabled = true;
    }

    void PrepareInstancing()
    {
        if (instancingPrepared) return;

        if (instanceVBO == 0) {
            glGenBuffers(1, &instanceVBO);
        }

        EnableInstancing(instanceVBO);

        instancingPrepared = true;
    }

private:
    void setupMesh(const MeshData& data)
    {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, data.vertices.size() * sizeof(Vertex), &data.vertices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, data.indices.size() * sizeof(unsigned int), &data.indices[0], GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));
        glEnableVertexAttribArray(5);
        glVertexAttribIPointer(5, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, m_BoneIDs));
        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, m_Weights));

        glBindVertexArray(0);
    }

 
    
};
#endif