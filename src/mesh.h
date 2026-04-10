#ifndef MESH_H
#define MESH_H

#include <glad/glad.h> // holds all OpenGL type declarations

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <shader.h>

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
    std::shared_ptr<unsigned int> id;
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
    vector<Texture>      textures;
    vector<float>        ringIDs;

    std::shared_ptr<RenderData> renderData;

    GLuint cubemapTexture = 0;
    bool reflect = false;
    unsigned int numPoints = 0;
    unsigned int instanceVBO = 0;

    // constructor 1
    Mesh(vector<Vertex> vertices, vector<unsigned int> indices, vector<Texture> textures)
    {
        this->vertices = std::move(vertices);
        this->indices = std::move(indices);
        this->textures = std::move(textures);

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
    void Draw(Shader& shader, GLsizei instanceCount = 0)
    {
        // bind appropriate textures
        unsigned int diffuseNr = 1;
        unsigned int specularNr = 1;
        unsigned int normalNr = 1;
        unsigned int heightNr = 1;


        if (reflect && cubemapTexture)
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
            glUniform1i(glGetUniformLocation(shader.shaderProgramID, "skybox"), 0);
        }
        else
        {
            for (unsigned int i = 0; i < textures.size(); i++)
            {
                glActiveTexture(GL_TEXTURE0 + i);
                string number;
                string name = textures[i].type;
                string uniformName;

                if (name == "texture_diffuse")
                {
                    uniformName = "material.diffuse";
                    number = std::to_string(diffuseNr++);
                }
                else if (name == "texture_specular")
                {
                    uniformName = "material.specular";
                    number = std::to_string(specularNr++);
                }
                else if (name == "texture_normal")
                {
                    uniformName = "material.normal";
                    number = std::to_string(normalNr++);
                }
                else if (name == "texture_height")
                {
                    uniformName = "material.height";
                    number = std::to_string(heightNr++);
                }

                glUniform1i(glGetUniformLocation(shader.shaderProgramID, (uniformName + number).c_str()), i);
                glBindTexture(GL_TEXTURE_2D, *(textures[i].id));
            }
        }

        glUniform1f(glGetUniformLocation(shader.shaderProgramID, "material.shininess"), 32.0f);

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

        glActiveTexture(GL_TEXTURE0);
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