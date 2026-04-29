#version 460 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

out vec4 FragColor;

struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct Material {
    sampler2D diffuse1;
    sampler2D specular1;
    float     shininess;
    bool      hasDiffuseMap;
    vec3      diffuseColor;
};

#define NR_POINT_LIGHTS 4
#define NR_SPOT_LIGHTS 4

uniform DirLight dirLight;
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform SpotLight spotLights[NR_SPOT_LIGHTS];

uniform Material material;
uniform vec3 viewPos;

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 baseColor);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 baseColor);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 baseColor);

void main()
{
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    vec3 baseColor;
    if (material.hasDiffuseMap) {
        baseColor = texture(material.diffuse1, TexCoords).rgb;
    } else {
        baseColor = material.diffuseColor;
    }

    vec3 result = vec3(0.0);

    if (length(dirLight.direction) > 0.0) {
        result += CalcDirLight(dirLight, norm, viewDir, baseColor);
    }

    for(int i = 0; i < NR_POINT_LIGHTS; i++) {
        if (pointLights[i].constant > 0.0) {
            result += CalcPointLight(pointLights[i], norm, FragPos, viewDir, baseColor);
        }
    }

    for(int i = 0; i < NR_SPOT_LIGHTS; i++) {
        if (spotLights[i].constant > 0.0) {
            result += CalcSpotLight(spotLights[i], norm, FragPos, viewDir, baseColor);
        }
    }

    float NdotV = max(dot(viewDir, norm), 0.0);
    float n_r = 3.0;

    float rimIntensity = pow((1.0 - NdotV), n_r);
    vec3 rimLightColor = vec3(0.0, 0.8, 1.0);
    vec3 rim = rimLightColor * rimIntensity;

    result += rim;

    FragColor = vec4(result, 1.0);
}

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 baseColor)
{
    vec3 lightDir = normalize(-light.direction);

    float diff = max(dot(normal, lightDir), 0.0);

    vec3 ambient = light.ambient * baseColor;
    vec3 diffuse = light.diffuse * baseColor * diff * 0.3;

    return (ambient + diffuse);
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 baseColor)
{
    vec3 lightDir = normalize(light.position - fragPos);

    float diff = max(dot(normal, lightDir), 0.0);

    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    vec3 ambient = light.ambient * baseColor;
    vec3 diffuse = light.diffuse * baseColor * diff * 0.3;

    ambient *= attenuation;
    diffuse *= attenuation;

    return (ambient + diffuse);
}

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 baseColor)
{
    vec3 lightDir = normalize(light.position - fragPos);

    float diff = max(dot(normal, lightDir), 0.0);

    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    vec3 spotDir = normalize(-light.direction);
    float theta = dot(lightDir, spotDir);
    float epsilon = light.cutOff - light.outerCutOff;

    float intensity = 0.0;
    if (epsilon > 0.0) {
        intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    }

    vec3 ambient = light.ambient * baseColor;
    vec3 diffuse = light.diffuse * baseColor * diff * 0.3;

    ambient *= attenuation;
    diffuse *= attenuation * intensity;

    return (ambient + diffuse);
}