//
// Created by kubka on 11.05.2026.
//

#ifndef MIMICRY_EXPERIMENTS_DEBUGDRAWSYSTEM_H
#define MIMICRY_EXPERIMENTS_DEBUGDRAWSYSTEM_H


#include <vector>
#include <glm/glm.hpp>

#include "shader.h"

struct DebugLine {
    glm::vec3 a;
    glm::vec3 b;
    glm::vec4 color;
};

class DebugDrawSystem {
public:
    static void Init();
    static void AddLine(const glm::vec3& a,
                        const glm::vec3& b,
                        const glm::vec4& color);
    static void Flush(const glm::mat4& vp);
    static void AddAABB(const glm::vec3& min, const glm::vec3& max, const glm::vec4& color);

    static void DrawAABBImmediate(const glm::vec3& min, const glm::vec3& max, const glm::mat4& vp);
    static void DrawAABBSolid(const glm::vec3& min, const glm::vec3& max, const glm::mat4& vp);
private:
    static std::vector<DebugLine> lines;
    static unsigned int VAO, VBO;
    static Shader* debugShader;
    static unsigned int solidVAO, solidVBO, solidEBO;
    static void InitSolid();
};


#endif //MIMICRY_EXPERIMENTS_DEBUGDRAWSYSTEM_H
