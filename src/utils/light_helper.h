#ifndef LIGHT_HELPER_H
#define LIGHT_HELPER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class LightHelper {
public:


    // wysy³anie do shadera
    static void Apply(TransformComponent& transform, LightComponent& light, Shader& shader) {

        if (light.type == Directional)
        {
            shader.setVec3("dirLight.direction", light.direction);
            shader.setVec3("dirLight.ambient", light.isOn ? light.ambient : glm::vec3(0.0f));
            shader.setVec3("dirLight.diffuse", light.isOn ? light.diffuse : glm::vec3(0.0f));
            shader.setVec3("dirLight.specular", light.isOn ? light.specular : glm::vec3(0.0f));
        }
        else if (light.type == Point)
        {
            std::string base = "pointLights[" + std::to_string(light.index) + "]";

            shader.setVec3(base + ".position", transform.position);
            shader.setVec3(base + ".ambient", light.isOn ? light.ambient : glm::vec3(0.0f));
            shader.setVec3(base + ".diffuse", light.isOn ? light.diffuse : glm::vec3(0.0f));
            shader.setVec3(base + ".specular", light.isOn ? light.specular : glm::vec3(0.0f));

            shader.setFloat(base + ".constant", light.constant);
            shader.setFloat(base + ".linear", light.linear);
            shader.setFloat(base + ".quadratic", light.quadratic);
        }
        else if (light.type == Spot)
        {
            std::string base = "spotLights[" + std::to_string(light.index) + "]";

            shader.setVec3(base + ".position", transform.position);
            shader.setVec3(base + ".direction", light.direction);
            shader.setVec3(base + ".ambient", light.isOn ? light.ambient : glm::vec3(0.0f));
            shader.setVec3(base + ".diffuse", light.isOn ? light.diffuse : glm::vec3(0.0f));
            shader.setVec3(base + ".specular", light.isOn ? light.specular : glm::vec3(0.0f));

            shader.setFloat(base + ".constant", light.constant);
            shader.setFloat(base + ".linear", light.linear);
            shader.setFloat(base + ".quadratic", light.quadratic);
            shader.setFloat(base + ".cutOff", light.cutOff);
            shader.setFloat(base + ".outerCutOff", light.outerCutOff);
        }
    }
};

#endif