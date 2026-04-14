#ifndef SPRITE_SYSTEM_H
#define SPRITE_SYSTEM_H

#include "core/ecs.h"
#include "core/component.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

#include <spdlog/spdlog.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <map>
#include <string>
#include <shader.h>
#include <freetype/freetype.h>


class SpriteSystem : public System {
public:

    struct Character {
        unsigned int textureID;
        glm::ivec2   size;
        glm::ivec2   bearing;
        unsigned int advance;
    };

private:
    Query<SpriteComponent>* spriteQuery;
    GLFWwindow* window = nullptr;

    GLuint VAO = 0, VBO = 0;
    std::unique_ptr<Shader> spriteShader;

    GLuint textVAO = 0, textVBO = 0;
    std::unique_ptr<Shader> textShader;

    std::map<std::string, std::map<char, Character>> fontCache;

    FT_Library ft = nullptr;

public:
    SpriteSystem(ECS& ecs, GLFWwindow* win) : window(win)
    {
        spriteQuery = ecs.CreateQuery<SpriteComponent>();

        if (FT_Init_FreeType(&ft))
            spdlog::error("SpriteSystem: FreeType does not work");

        Init();
    }

    ~SpriteSystem()
    {
        if (VAO)     glDeleteVertexArrays(1, &VAO);
        if (VBO)     glDeleteBuffers(1, &VBO);
        if (textVAO) glDeleteVertexArrays(1, &textVAO);
        if (textVBO) glDeleteBuffers(1, &textVBO);

        for (auto& [key, chars] : fontCache)
            for (auto& [c, ch] : chars)
                glDeleteTextures(1, &ch.textureID);

        if (ft) FT_Done_FreeType(ft);
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
        textShader = std::make_unique<Shader>(
            "res/shaders/text.vert",
            "res/shaders/text.frag"
        );
        SetupSpriteQuad();
        SetupTextQuad();
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

        glm::mat4 proj = glm::ortho(
            0.0f, (float)display_w,
            (float)display_h, 0.0f,
            -1.0f, 1.0f
        );

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        std::vector<SpriteComponent*> sorted;
        for (size_t i = 0; i < spriteQuery->gameobjects.size(); i++)
        {
            if (sprites[i] && sprites[i]->isVisible)
                sorted.push_back(sprites[i]);
        }
        std::sort(sorted.begin(), sorted.end(),
            [](SpriteComponent* a, SpriteComponent* b) {
                return a->layer < b->layer;
            });

        for (SpriteComponent* sprite : sorted)
        {
            UpdateAnimation(*sprite, deltaTime);

            if (!sprite->sprites.empty())
                RenderSprite(*sprite, proj, display_w, display_h);

            if (sprite->textEnabled && !sprite->text.empty())
                RenderText(*sprite, proj);
        }

        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
    }

private:

    void UpdateAnimation(SpriteComponent& sprite, float dt)
    {
        if (sprite.scrollEnabled)
            sprite.scrollOffset += sprite.scrollSpeed * dt;

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

    void RenderSprite(SpriteComponent& sprite, const glm::mat4& proj, int w, int h)
    {
        unsigned int texID = sprite.currentTextureID();
        if (texID == 0) return;

        glViewport(0, 0, w, h);

        spriteShader->use();
        spriteShader->setMat4("projection", proj);
        spriteShader->setFloat("opacity", sprite.opacity);
        spriteShader->setInt("spriteTexture", 0);
        spriteShader->setVec2("scrollOffset", sprite.scrollOffset);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(sprite.screenPosition, 0.0f));
        model = glm::scale(model, glm::vec3(sprite.size, 1.0f));
        spriteShader->setMat4("model", model);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texID);

        if (sprite.scrollEnabled)
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        }

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }

    void RenderText(SpriteComponent& sprite, const glm::mat4& proj)
    {
        auto& chars = GetOrLoadFont(sprite.fontPath, (int)sprite.fontSize);

        textShader->use();
        textShader->setMat4("projection", proj);
        textShader->setVec3("textColor", sprite.textColor);
        textShader->setInt("text", 0);

        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(textVAO);

        float x = sprite.screenPosition.x + sprite.textOffset.x;
        float y = sprite.screenPosition.y + sprite.textOffset.y;

        for (char c : sprite.text)
        {
            auto it = chars.find(c);
            if (it == chars.end()) continue;

            const Character& ch = it->second;

            float xpos = x + ch.bearing.x;
            float ypos = y + ((int)sprite.fontSize - ch.bearing.y);

            float w = (float)ch.size.x;
            float h = (float)ch.size.y;

            float verts[6][4] = {
                { xpos,     ypos + h, 0.0f, 1.0f },
                { xpos + w, ypos,     1.0f, 0.0f },
                { xpos,     ypos,     0.0f, 0.0f },

                { xpos,     ypos + h, 0.0f, 1.0f },
                { xpos + w, ypos + h, 1.0f, 1.0f },
                { xpos + w, ypos,     1.0f, 0.0f },
            };

            glBindTexture(GL_TEXTURE_2D, ch.textureID);
            glBindBuffer(GL_ARRAY_BUFFER, textVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            x += (ch.advance >> 6);
        }

        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    std::map<char, Character>& GetOrLoadFont(const std::string& path, int size)
    {
        std::string key = path + ":" + std::to_string(size);

        auto it = fontCache.find(key);
        if (it != fontCache.end())
            return it->second;

        std::map<char, Character> chars;

        FT_Face face;
        if (FT_New_Face(ft, path.c_str(), 0, &face))
        {
            spdlog::error("SpriteSystem: cant load font {}", path);
            fontCache[key] = chars;
            return fontCache[key];
        }

        FT_Set_Pixel_Sizes(face, 0, size);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        for (unsigned char c = 0; c < 128; c++)
        {
            if (FT_Load_Char(face, c, FT_LOAD_RENDER))
            {
                spdlog::warn("SpriteSystem: character skipped '{}'", c);
                continue;
            }

            unsigned int tex;
            glGenTextures(1, &tex);
            glBindTexture(GL_TEXTURE_2D, tex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED,
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows,
                0, GL_RED, GL_UNSIGNED_BYTE,
                face->glyph->bitmap.buffer
            );
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            chars[c] = {
                tex,
                glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                (unsigned int)face->glyph->advance.x
            };
        }

        FT_Done_Face(face);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

        fontCache[key] = std::move(chars);
        spdlog::info("SpriteSystem: font loaded {}:{}", path, size);
        return fontCache[key];
    }

    void SetupSpriteQuad()
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

    void SetupTextQuad()
    {
        glGenVertexArrays(1, &textVAO);
        glGenBuffers(1, &textVBO);
        glBindVertexArray(textVAO);
        glBindBuffer(GL_ARRAY_BUFFER, textVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
        glBindVertexArray(0);
    }
};

#endif