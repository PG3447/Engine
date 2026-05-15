#ifndef MATERIAL_H
#define MATERIAL_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>
#include "shader.h"

class Material {
public:
    Shader* shader = nullptr;

    GLuint diffuseMap = 0;
    GLuint specularMap = 0;
    GLuint normalMap = 0;
    bool transparent = false;

    glm::vec4 baseColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    float shininess = 32.0f;

    Material() = default;

    Material(Shader* s, GLuint diffuse = 0, GLuint specular = 0, GLuint normal = 0)
        : shader(s), diffuseMap(diffuse), specularMap(specular), normalMap(normal) {
    }

    void Apply() const {
        Shader* activeShader = shader;

        if (!activeShader) {
            spdlog::warn("Material probuje zostac wyrenderowany, ale nie ma przypisanego shadera!");
            return;
        }

        activeShader->use();

        if (diffuseMap != 0) {
            glActiveTexture(GL_TEXTURE0);
            activeShader->setInt("material.diffuse1", 0);
            glBindTexture(GL_TEXTURE_2D, diffuseMap);
            activeShader->setBool("material.hasDiffuseMap", true);
        }
        else {
            activeShader->setBool("material.hasDiffuseMap", false);
        }

        activeShader->setVec4("material.diffuseColor", baseColor);

        glActiveTexture(GL_TEXTURE1);
        activeShader->setInt("material.specular1", 1);
        glBindTexture(GL_TEXTURE_2D, specularMap);

        glActiveTexture(GL_TEXTURE2);
        activeShader->setInt("material.normalMap", 2);
        glBindTexture(GL_TEXTURE_2D, normalMap);

        activeShader->setFloat("material.shininess", shininess);

        if (normalMap != 0) {
            activeShader->setBool("material.hasNormalMap", true);
        }
        else {
            activeShader->setBool("material.hasNormalMap", false);
        }

        glActiveTexture(GL_TEXTURE0);
    }
};

#endif