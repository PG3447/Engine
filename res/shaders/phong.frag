#version 460 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float     shininess;
};

struct SpotLight {
    vec3  position;
    vec3  direction;
    float cutOff;
    float outerCutOff;
    vec3  ambient;
    vec3  diffuse;
    vec3  specular;
};

uniform DirLight  dirLight;
uniform Material  material;
uniform vec3      viewPos;
uniform SpotLight spotLight;

void main()
{
    vec3 norm    = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    vec3 texDiffuse  = vec3(0.3);
    vec3 texSpecular = vec3(0.3);

    vec3 lightDir   = normalize(-dirLight.direction);
    float diff      = max(dot(norm, lightDir), 0.0);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec      = pow(max(dot(norm, halfwayDir), 0.0), material.shininess);

    vec3 ambient  = dirLight.ambient  * texDiffuse;
    vec3 diffuse  = dirLight.diffuse  * diff * texDiffuse;
    vec3 specular = dirLight.specular * spec * texSpecular;

    vec3  spotDir   = normalize(spotLight.position - FragPos);
    float theta     = dot(spotDir, normalize(-spotLight.direction));
    float epsilon   = spotLight.cutOff - spotLight.outerCutOff;
    float intensity = clamp((theta - spotLight.outerCutOff) / epsilon, 0.0, 1.0);

    float spotDiff  = max(dot(norm, spotDir), 0.0);
    vec3 spotHalf   = normalize(spotDir + viewDir);
    float spotSpec  = pow(max(dot(norm, spotHalf), 0.0), material.shininess);

    vec3 spotDiffuse  = spotLight.diffuse  * intensity * spotDiff * texDiffuse;
    vec3 spotSpecular = spotLight.specular * intensity * spotSpec * texSpecular;

    vec3 result = ambient + diffuse + specular + spotDiffuse + spotSpecular;
    FragColor = vec4(result, 1.0);
}