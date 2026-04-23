#ifndef MATERIAL_H
#define MATERIAL_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include "shader.h"

class Material {
public:
    GLuint diffuseMap = 0;
    GLuint specularMap = 0;
    GLuint normalMap = 0;

    glm::vec3 diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);

    float shininess = 32.0f;

    void Apply(Shader& shader) const {
        shader.use();

        if (diffuseMap != 0) {
            glActiveTexture(GL_TEXTURE0);
            shader.setInt("material.diffuse1", 0);
            glBindTexture(GL_TEXTURE_2D, diffuseMap);
            shader.setBool("material.hasDiffuseMap", true);
        }
        else {
            shader.setBool("material.hasDiffuseMap", false);
        }

        shader.setVec3("material.diffuseColor", diffuseColor);

        glActiveTexture(GL_TEXTURE1);
        shader.setInt("material.specular1", 1);
        glBindTexture(GL_TEXTURE_2D, specularMap);

        glActiveTexture(GL_TEXTURE2);
        shader.setInt("material.normalMap", 2);
        glBindTexture(GL_TEXTURE_2D, normalMap);

        shader.setFloat("material.shininess", shininess);

        if (normalMap != 0) {
            shader.setBool("material.hasNormalMap", true);
        }
        else {
            shader.setBool("material.hasNormalMap", false);
        }

        glActiveTexture(GL_TEXTURE0);
    }
};

#endif