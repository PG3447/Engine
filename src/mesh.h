#ifndef MESH_H
#define MESH_H

#include <glad/glad.h> // holds all OpenGL type declarations

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <shader.h>
#include "material.h"

#include <string>
#include <vector>
#include <memory>

using namespace std;

#define MAX_BONE_INFLUENCE 4

struct RenderData {
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    unsigned int EBO = 0;

    ~RenderData() {
        if (VAO != 0) glDeleteVertexArrays(1, &VAO);
        if (VBO != 0) glDeleteBuffers(1, &VBO);
        if (EBO != 0) glDeleteBuffers(1, &EBO);
    }
};



struct Vertex {
    // position
    glm::vec3 Position;
    // normal
    glm::vec3 Normal;
    // texCoords
    glm::vec2 TexCoords;
    // tangent
    glm::vec3 Tangent;
    // bitangent
    glm::vec3 Bitangent;
    //bone indexes which will influence this vertex
    int m_BoneIDs[MAX_BONE_INFLUENCE];
    //weights from each bone
    float m_Weights[MAX_BONE_INFLUENCE];
};

struct Texture {
    GLuint id;
    string type;
    string path;
};

class Mesh {
public:

    enum MeshType {
        MESH_TRIANGLES = 0,
        MESH_LINES = 1,
        MESH_POINTS = 2
    };

    MeshType meshType = MESH_TRIANGLES;
    // mesh Data
    vector<Vertex>       vertices;
    vector<unsigned int> indices;
    std::shared_ptr<Material> material;
    vector<float>        ringIDs;

    std::shared_ptr<RenderData> renderData;

    GLuint cubemapTexture = 0;
    bool reflect = false;
    bool instancingEnabled = false;
    unsigned int numPoints = 0;
    unsigned int instanceVBO = 0;

    // constructor 1
    Mesh(vector<Vertex> vertices, vector<unsigned int> indices, std::shared_ptr<Material> mat)
    {
        this->vertices = std::move(vertices);
        this->indices = std::move(indices);
        this->material = mat;

        this->renderData = std::make_shared<RenderData>();
        setupMesh();
    }

    // constructor 2 (Points)
    Mesh(const std::vector<float>& ringIDs)
    {
        this->meshType = MESH_POINTS;
        this->numPoints = (unsigned int)ringIDs.size();

        this->renderData = std::make_shared<RenderData>();

        glGenVertexArrays(1, &renderData->VAO);
        glGenBuffers(1, &renderData->VBO);

        glBindVertexArray(renderData->VAO);
        glBindBuffer(GL_ARRAY_BUFFER, renderData->VBO);
        glBufferData(GL_ARRAY_BUFFER, numPoints * sizeof(float), ringIDs.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, sizeof(float), (void*)0);

        glBindVertexArray(0);
    }

    void updateSphereMesh(int rings, int sectors)
    {
        std::vector<float> ringIDs;
        for (int r = 0; r < rings; ++r)
            ringIDs.push_back((float)r);

        glBindBuffer(GL_ARRAY_BUFFER, renderData->VBO);
        glBufferData(GL_ARRAY_BUFFER, ringIDs.size() * sizeof(float), ringIDs.data(), GL_STATIC_DRAW);
        numPoints = (unsigned int)ringIDs.size();
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    // render the mesh
    void Draw(GLsizei instanceCount = 0, Material* overrideMaterial = nullptr)
    {
        Material* activeMaterial = overrideMaterial ? overrideMaterial : material.get();
        Shader* activeShader = activeMaterial ? activeMaterial->shader : nullptr;

        if (reflect && cubemapTexture && activeShader)
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
            activeShader->use();
            activeShader->setInt("skybox", 0);
        }
        else if (activeMaterial)
        {
            activeMaterial->Apply();
        }

        glBindVertexArray(renderData->VAO);

        if (instanceCount == 0)
        {
            if (meshType == MESH_LINES)
                glDrawElements(GL_LINE_LOOP, (unsigned int)indices.size(), GL_UNSIGNED_INT, 0);
            else if (meshType == MESH_POINTS)
                glDrawArrays(GL_POINTS, 0, (GLsizei)numPoints);
            else
                glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
        }
        else
        {
            if (meshType == MESH_LINES)
                glDrawElementsInstanced(GL_LINE_LOOP, (unsigned int)indices.size(), GL_UNSIGNED_INT, 0, instanceCount);
            else if (meshType == MESH_POINTS)
                glDrawArraysInstanced(GL_POINTS, 0, (GLsizei)numPoints, instanceCount);
            else
                glDrawElementsInstanced(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0, instanceCount);
        }
        glBindVertexArray(0);
    }


    void EnableInstancing(unsigned int vbo)
    {
        if (instancingEnabled)
            return;

        glBindVertexArray(renderData->VAO);

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

private:

    // initializes all the buffer objects/arrays
    void setupMesh()
    {
        glGenVertexArrays(1, &renderData->VAO);
        glGenBuffers(1, &renderData->VBO);
        glGenBuffers(1, &renderData->EBO);

        glBindVertexArray(renderData->VAO);
        glBindBuffer(GL_ARRAY_BUFFER, renderData->VBO);

        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderData->EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

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