//
// Created by kubka on 11.05.2026.
//

#ifndef MIMICRY_EXPERIMENTS_DEBUGDRAWSYSTEM_H
#define MIMICRY_EXPERIMENTS_DEBUGDRAWSYSTEM_H


#include <vector>
#include <glm/glm.hpp>

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

private:
    static std::vector<DebugLine> lines;
    static unsigned int VAO, VBO;
};


#endif //MIMICRY_EXPERIMENTS_DEBUGDRAWSYSTEM_H
