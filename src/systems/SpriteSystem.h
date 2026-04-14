#ifndef SPRITE_SYSTEM_H
#define SPRITE_SYSTEM_H

#include "core/ecs.h"
#include "core/component.h"
#include "resource_manager.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

class SpriteSystem : public System {
private:
    Query<SpriteComponent>* spriteQuery;

    GLFWwindow* window = nullptr;

    GLuint VAO = 0;
    GLuint VBO = 0;

    std::unique_ptr<Shader> spriteShader;

public:
    SpriteSystem(ECS& ecs, GLFWwindow* win) : window(win)
    {
        spriteQuery = ecs.CreateQuery<SpriteComponent>();
        Init();
    }

    ~SpriteSystem()
    {
        if (VAO) glDeleteVertexArrays(1, &VAO);
        if (VBO) glDeleteBuffers(1, &VBO);
    }

    void OnGameObjectUpdated(GameObject* e) override
    {
        spriteQuery->OnGameObjectUpdated(e);
    }

    void Init()
    {
        spriteShader = std::make_unique<Shader>(
            "res/shaders/sprite.vert",
            "res/shaders/sprite.frag"
        );
        SetupQuad();
    }

    void Update(ECS& ecs) override
    {
        auto& sprites = std::get<0>(spriteQuery->componentsVectors);

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        
        float dt = static_cast<float>(glfwGetTime());
        static float lastTime = dt;
        float deltaTime = dt - lastTime;
        lastTime = dt;

        glViewport(0, 0, display_w, display_h);

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glm::mat4 proj = glm::ortho(
            0.0f, (float)display_w,
            (float)display_h, 0.0f,
            -1.0f, 1.0f
        );

        spriteShader->use();
        spriteShader->setMat4("projection", proj);
        spriteShader->setInt("spriteTexture", 0);

        for (size_t i = 0; i < spriteQuery->gameobjects.size(); i++)
        {
            SpriteComponent* sprite = sprites[i];
            if (!sprite || !sprite->isVisible || sprite->sprites.empty())
                continue;

            UpdateAnimation(*sprite, deltaTime);
            RenderSprite(*sprite);
        }

        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
    }

private:
    void UpdateAnimation(SpriteComponent& sprite, float dt)
    {
        if (!sprite.isAnimating || sprite.totalFrames() <= 1) return;

        sprite.elapsedTime += dt;
        if (sprite.elapsedTime >= sprite.frameDuration)
        {
            sprite.elapsedTime -= sprite.frameDuration;
            sprite.currentSprite++;

            if (sprite.currentSprite >= sprite.totalFrames())
                sprite.currentSprite = sprite.loop ? 0 : sprite.totalFrames() - 1;
        }
    }

    void RenderSprite(SpriteComponent& sprite)
    {
        unsigned int texID = sprite.currentTextureID();
        if (texID == 0) return;

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(sprite.screenPosition, 0.0f));
        model = glm::scale(model, glm::vec3(sprite.size, 1.0f));

        spriteShader->setMat4("model", model);
        spriteShader->setFloat("opacity", sprite.opacity);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texID);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }

    void SetupQuad()
    {
        float vertices[] = {
            // xy        // uv
            0.0f, 1.0f,  0.0f, 1.0f,
            1.0f, 0.0f,  1.0f, 0.0f,
            0.0f, 0.0f,  0.0f, 0.0f,

            0.0f, 1.0f,  0.0f, 1.0f,
            1.0f, 1.0f,  1.0f, 1.0f,
            1.0f, 0.0f,  1.0f, 0.0f,
        };

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

        glBindVertexArray(0);
    }
};

#endif