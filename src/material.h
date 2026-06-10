#ifndef MATERIAL_H
#define MATERIAL_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>
#include "shader.h"

enum class SurfaceType {
    Opaque,
    Transparent
};

class Material {
public:
    Shader* shader = nullptr;

    SurfaceType surfaceType = SurfaceType::Opaque;

    std::vector<uint32_t> materialID;

    GLuint diffuseMap = 0;
    GLuint specularMap = 0;
    GLuint metallicRoughnessMap = 0;
    GLuint aoMap = 0;
    GLuint normalMap = 0;
    bool aoInMetallicRoughness = false;

    glm::vec4 diffuseColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    float shininess = 32.0f;

    Material() = default;

    Material(Shader* s, GLuint diffuse = 0, GLuint specular = 0, GLuint normal = 0)
        : shader(s), diffuseMap(diffuse), specularMap(specular), normalMap(normal) {
    }
   

    void setMaterialId(int passId, uint32_t id)
    {
        if (passId >= static_cast<int>(materialID.size()))
            materialID.resize(passId + 1, UINT32_MAX);

        materialID[passId] = id;
    }

    uint32_t getMaterialId(int passId) const
    {
        if (passId >= static_cast<int>(materialID.size()))
            return UINT32_MAX;

        return materialID[passId];
    }

    bool hasMaterial(int passId) const
    {
        return getMaterialId(passId) != UINT32_MAX;
    }

    void removeMaterial(int passId)
    {
        setMaterialId(passId, UINT32_MAX);
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

        activeShader->setVec3("material.diffuseColor", diffuseColor);

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