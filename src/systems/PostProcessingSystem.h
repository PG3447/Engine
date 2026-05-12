#ifndef POST_PROCESSING_SYSTEM_H
#define POST_PROCESSING_SYSTEM_H

#include "core/ecs.h"
#include "render_system.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <shader.h>
#include <memory>

class PostProcessingSystem : public System {
private:
    GLFWwindow* window;
    RenderSystem* renderSystem;

    std::unique_ptr<Shader> postShader;

    GLuint quadVAO = 0;
    GLuint quadVBO = 0;

    float gamma = 1.0f;

    void InitQuad() {
        // Fullscreen quad — dwa trójkąty pokrywające NDC [-1,1]
        float vertices[] = {
            // pos (xy)    // uv
            -1.f,  1.f,   0.f, 1.f,
            -1.f, -1.f,   0.f, 0.f,
             1.f, -1.f,   1.f, 0.f,

            -1.f,  1.f,   0.f, 1.f,
             1.f, -1.f,   1.f, 0.f,
             1.f,  1.f,   1.f, 1.f,
        };

        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);

        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

        glBindVertexArray(0);
    }

public:
    PostProcessingSystem(ECS& ecs, GLFWwindow* win)
    {
        renderSystem = ecs.GetSystem<RenderSystem>();
        window = win;
        postShader = std::make_unique<Shader>(
            "res/shaders/postprocess.vert",
            "res/shaders/postprocess.frag"
        );
        InitQuad();
    }

    ~PostProcessingSystem() {
        glDeleteVertexArrays(1, &quadVAO);
        glDeleteBuffers(1, &quadVBO);
    }

    void OnGameObjectUpdated(GameObject* e) override {
        //unused
    }

    void Update(ECS& ecs, float dt) override {
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, display_w, display_h);
        glClear(GL_COLOR_BUFFER_BIT);

        glDisable(GL_DEPTH_TEST);

        postShader->use();
        postShader->setInt("screenTexture", 0);
        postShader->setFloat("gamma", gamma);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, renderSystem->GetSceneTexture());

        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        glEnable(GL_DEPTH_TEST);
    }

    [[nodiscard]] float get_gamma() const {
        return gamma;
    }

    void set_gamma(float gamma) {
        this->gamma = gamma;
    }
};

#endif