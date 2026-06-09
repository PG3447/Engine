//
// Created by kubka on 11.05.2026.
//

#include "DebugDrawSystem.h"


#include <glad/glad.h>
#include "shader.h"

std::vector<DebugLine> DebugDrawSystem::lines;
unsigned int DebugDrawSystem::VAO = 0;
unsigned int DebugDrawSystem::VBO = 0;

Shader* DebugDrawSystem::debugShader = nullptr;

struct DebugVertex {
    glm::vec3 pos;
    glm::vec4 color;
};

void DebugDrawSystem::Init()
{
    debugShader = new Shader(
        "res/shaders/debug_line.vert",
        "res/shaders/debug_line.frag"
    );
    InitSolid();

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(DebugVertex) * 2, nullptr, GL_DYNAMIC_DRAW);

    // pozycja
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(DebugVertex), (void*)0);
    glEnableVertexAttribArray(0);

    // kolor
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(DebugVertex), (void*)sizeof(glm::vec3));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    //glBindVertexArray(VAO);
    //glBindBuffer(GL_ARRAY_BUFFER, VBO);
    //glBufferData(GL_ARRAY_BUFFER,
    //    sizeof(glm::vec3) * 2,
    //    nullptr,
    //    GL_DYNAMIC_DRAW);

    //glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
    //    sizeof(glm::vec3), (void*)0);
    //glEnableVertexAttribArray(0);

    //glBindVertexArray(0);
}
void DebugDrawSystem::AddLine(const glm::vec3& a,
                             const glm::vec3& b,
                             const glm::vec4& color)
{
    lines.push_back({ a, b, color });
}

void DebugDrawSystem::Flush(const glm::mat4& vp)
{
    if (lines.empty()) return;

    // bufor wszystkich wierzchołków
    std::vector<DebugVertex> vertices;
    vertices.reserve(lines.size() * 2);

    for (const auto& l : lines) {
        vertices.push_back({ l.a, l.color });
        vertices.push_back({ l.b, l.color });
    }

    glDisable(GL_DEPTH_TEST);
    glLineWidth(1.0f);

    debugShader->use();
    debugShader->setMat4("uVP", vp);
    debugShader->setBool("uUseUniformColor", false);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(DebugVertex), vertices.data(), GL_DYNAMIC_DRAW);

    glDrawArrays(GL_LINES, 0, (GLsizei)vertices.size());
    //for (const auto& l : lines) {
    //    debugShader->setVec4("uColor", l.color);

    //    glm::vec3 data[2] = { l.a, l.b };
    //    glBufferSubData(
    //        GL_ARRAY_BUFFER, 0,
    //        sizeof(data), data
    //    );
    //    glDrawArrays(GL_LINES, 0, 2);
    //}

    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);

    lines.clear();
}
void DebugDrawSystem::AddAABB(const glm::vec3& min, const glm::vec3& max, const glm::vec4& color) {
    glm::vec3 corners[8] = {
        {min.x, min.y, min.z}, {max.x, min.y, min.z},
        {min.x, max.y, min.z}, {max.x, max.y, min.z},
        {min.x, min.y, max.z}, {max.x, min.y, max.z},
        {min.x, max.y, max.z}, {max.x, max.y, max.z},
    };

    AddLine(corners[0], corners[1], color);
    AddLine(corners[0], corners[2], color);
    AddLine(corners[1], corners[3], color);
    AddLine(corners[2], corners[3], color);

    AddLine(corners[4], corners[5], color);
    AddLine(corners[4], corners[6], color);
    AddLine(corners[5], corners[7], color);
    AddLine(corners[6], corners[7], color);

    AddLine(corners[0], corners[4], color);
    AddLine(corners[1], corners[5], color);
    AddLine(corners[2], corners[6], color);
    AddLine(corners[3], corners[7], color);
}
void DebugDrawSystem::DrawAABBImmediate(const glm::vec3& min, const glm::vec3& max, const glm::mat4& vp)
{
    if (!debugShader) return;

    glm::vec3 corners[8] = {
        {min.x, min.y, min.z}, {max.x, min.y, min.z},
        {min.x, max.y, min.z}, {max.x, max.y, min.z},
        {min.x, min.y, max.z}, {max.x, min.y, max.z},
        {min.x, max.y, max.z}, {max.x, max.y, max.z},
    };

    static const int edges[24] = {
        0,1, 0,2, 1,3, 2,3,
        4,5, 4,6, 5,7, 6,7,
        0,4, 1,5, 2,6, 3,7
    };

    debugShader->use();
    debugShader->setMat4("uVP", vp);
    debugShader->setVec4("uColor", glm::vec4(0.0f));

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    for (int e = 0; e < 24; e += 2) {
        glm::vec3 data[2] = { corners[edges[e]], corners[edges[e+1]] };
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(data), data);
        glDrawArrays(GL_LINES, 0, 2);
    }

    glBindVertexArray(0);
}
unsigned int DebugDrawSystem::solidVAO = 0;
unsigned int DebugDrawSystem::solidVBO = 0;
unsigned int DebugDrawSystem::solidEBO = 0;

void DebugDrawSystem::InitSolid() {
    static const unsigned int indices[36] = {
        // przód (z = min)
        0, 2, 1,  1, 2, 3,
        // tył (z = max)
        5, 7, 4,  4, 7, 6,
        // lewo (x = min)
        4, 6, 0,  0, 6, 2,
        // prawo (x = max)
        1, 3, 5,  5, 3, 7,
        // dół (y = min)
        4, 0, 5,  5, 0, 1,
        // góra (y = max)
        2, 6, 3,  3, 6, 7,
    };

    glGenVertexArrays(1, &solidVAO);
    glGenBuffers(1, &solidVBO);
    glGenBuffers(1, &solidEBO);

    glBindVertexArray(solidVAO);

    // VBO — 8 wierzchołków, dynamic bo nadpisujemy per-query
    glBindBuffer(GL_ARRAY_BUFFER, solidVBO);
    glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

    // EBO — stały
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, solidEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glBindVertexArray(0);
}

void DebugDrawSystem::DrawAABBSolid(const glm::vec3& min, const glm::vec3& max, const glm::mat4& vp)
{
    if (!debugShader) return;

    glm::vec3 corners[8] = {
        {min.x, min.y, min.z}, {max.x, min.y, min.z},
        {min.x, max.y, min.z}, {max.x, max.y, min.z},
        {min.x, min.y, max.z}, {max.x, min.y, max.z},
        {min.x, max.y, max.z}, {max.x, max.y, max.z}
    };

    glBindBuffer(GL_ARRAY_BUFFER, solidVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, 8 * sizeof(glm::vec3), corners);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    debugShader->use();
    debugShader->setMat4("uVP", vp);
    debugShader->setBool("uUseUniformColor", true);
    debugShader->setVec4("uColor", glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));

    glBindVertexArray(solidVAO);
    glDisableVertexAttribArray(1);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}