#ifndef LIGHT_HELPER_H
#define LIGHT_HELPER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class LightHelper {
public:


    // wysy³anie do shadera
    static void Apply(const LightComponent& light, Shader& shader) {

        if (type == Directional)
        {
            shader.setVec3("dirLight.direction", direction);
            shader.setVec3("dirLight.ambient", isOn ? ambient : glm::vec3(0.0f));
            shader.setVec3("dirLight.diffuse", isOn ? diffuse : glm::vec3(0.0f));
            shader.setVec3("dirLight.specular", isOn ? specular : glm::vec3(0.0f));
        }
        else if (type == Point)
        {
            std::string base = "pointLights[" + std::to_string(index) + "]";

            shader.setVec3(base + ".position", position);
            shader.setVec3(base + ".ambient", isOn ? ambient : glm::vec3(0.0f));
            shader.setVec3(base + ".diffuse", isOn ? diffuse : glm::vec3(0.0f));
            shader.setVec3(base + ".specular", isOn ? specular : glm::vec3(0.0f));

            shader.setFloat(base + ".constant", constant);
            shader.setFloat(base + ".linear", linear);
            shader.setFloat(base + ".quadratic", quadratic);
        }
        else if (type == Spot)
        {
            std::string base = "spotLights[" + std::to_string(index) + "]";

            shader.setVec3(base + ".position", position);
            shader.setVec3(base + ".direction", direction);
            shader.setVec3(base + ".ambient", isOn ? ambient : glm::vec3(0.0f));
            shader.setVec3(base + ".diffuse", isOn ? diffuse : glm::vec3(0.0f));
            shader.setVec3(base + ".specular", isOn ? specular : glm::vec3(0.0f));

            shader.setFloat(base + ".constant", constant);
            shader.setFloat(base + ".linear", linear);
            shader.setFloat(base + ".quadratic", quadratic);
            shader.setFloat(base + ".cutOff", cutOff);
            shader.setFloat(base + ".outerCutOff", outerCutOff);
        }
    }
};

#endif