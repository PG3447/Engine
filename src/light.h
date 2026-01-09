#ifndef LIGHT_H
#define LIGHT_H

#include <glad/glad.h> // holds all OpenGL type declarations

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <shader.h>


class Light
{
public:
    enum LightType {
        Directional = 0,
        Point = 1,
        Spot = 2
    };

    // wspólne
    int index = 0;
    bool isOn = true;
    LightType type;

    glm::vec3 ambient = glm::vec3(0.05f);
    glm::vec3 diffuse = glm::vec3(1.0f);
    glm::vec3 specular = glm::vec3(1.0f);

    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);

    // attenuation (Point / Spot)
    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;

    // Spot
    float cutOff = glm::cos(glm::radians(12.5f));
    float outerCutOff = glm::cos(glm::radians(17.5f));


    Light(LightType type) : type(type) {}

    // wysy³anie do shadera
    void Apply(Shader& shader) {
        if (!isOn)
            return;

        if (type == Directional)
        {
            shader.setVec3("dirLight.direction", direction);
            shader.setVec3("dirLight.ambient", ambient);
            shader.setVec3("dirLight.diffuse", diffuse);
            shader.setVec3("dirLight.specular", specular);
        }
        else if (type == Point)
        {
            std::string base = "pointLights[" + std::to_string(index) + "]";

            shader.setVec3(base + ".position", position);
            shader.setVec3(base + ".ambient", ambient);
            shader.setVec3(base + ".diffuse", diffuse);
            shader.setVec3(base + ".specular", specular);

            shader.setFloat(base + ".constant", constant);
            shader.setFloat(base + ".linear", linear);
            shader.setFloat(base + ".quadratic", quadratic);
        }
        else if (type == Spot)
        {
            std::string base = "spotLights[" + std::to_string(index) + "]";

            shader.setVec3(base + ".position", position);
            shader.setVec3(base + ".direction", direction);
            shader.setVec3(base + ".ambient", ambient);
            shader.setVec3(base + ".diffuse", diffuse);
            shader.setVec3(base + ".specular", specular);

            shader.setFloat(base + ".constant", constant);
            shader.setFloat(base + ".linear", linear);
            shader.setFloat(base + ".quadratic", quadratic);
            shader.setFloat(base + ".cutOff", cutOff);
            shader.setFloat(base + ".outerCutOff", outerCutOff);
        }
    }
};

#endif