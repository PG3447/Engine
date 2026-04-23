#ifndef MATERIAL_H
#define MATERIAL_H

#include <glad/glad.h>
#include "shader.h"

class Material {
public:
    GLuint diffuseMap = 0;
    GLuint specularMap = 0;
    GLuint normalMap = 0;

    float shininess = 32.0f;

    void Apply(Shader& shader) const {
        shader.use();

        glActiveTexture(GL_TEXTURE0);
        shader.setInt("material.diffuse1", 0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap);

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