//
// Created by kubka on 11.05.2026.
//

#include "DebugDrawSystem.h"


#include <glad/glad.h>
#include "shader.h"

std::vector<DebugLine> DebugDrawSystem::lines;
unsigned int DebugDrawSystem::VAO = 0;
unsigned int DebugDrawSystem::VBO = 0;

static Shader* debugShader = nullptr;

void DebugDrawSystem::Init()
{
    debugShader = new Shader(
        "res/shaders/debug_line.vert",
        "res/shaders/debug_line.frag"
    );

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
        sizeof(glm::vec3) * 2,
        nullptr,
        GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
        sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
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

    glDisable(GL_DEPTH_TEST);
    glLineWidth(1.0f);

    debugShader->use();
    debugShader->setMat4("uVP", vp);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    for (const auto& l : lines) {
        debugShader->setVec4("uColor", l.color);

        glm::vec3 data[2] = { l.a, l.b };
        glBufferSubData(
            GL_ARRAY_BUFFER, 0,
            sizeof(data), data
        );
        glDrawArrays(GL_LINES, 0, 2);
    }

    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);

    lines.clear();
}
