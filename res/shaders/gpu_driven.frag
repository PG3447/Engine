#version 460 core
#extension GL_ARB_bindless_texture : require

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in mat3 TBN;
flat in uint materialID;


struct MaterialGPU
{
    uvec2 diffuseHandle;
    uvec2 specularHandle;
    uvec2 normalHandle;
    uint  padding;
    uint  padding2;
    vec4  diffuseColorAndShininess;
};

struct GPULight {
    vec4 position;  // xyz=pos,  w=type (0=dir, 1=point, 2=spot)
    vec4 direction; // xyz=dir,  w=unused
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    vec4 params1;   // x=constant, y=linear, z=quadratic, w=range
    vec4 params2;   // x=cutOff,   y=outerCutOff, z=enabled, w=unused
};

layout(std430, binding = 7) readonly buffer Materials
{
    MaterialGPU materials[];
};

layout(std430, binding = 8) readonly buffer Lights
{
    GPULight lights[];
};

uniform int  numLights;
uniform vec3 viewPos;


vec3 CalcDirLight  (in GPULight light, vec3 norm, vec3 viewDir, vec3 diffTex, vec3 specTex, float shininess);
vec3 CalcPointLight(in GPULight light, vec3 norm, vec3 viewDir, vec3 diffTex, vec3 specTex, float shininess);
vec3 CalcSpotLight (in GPULight light, vec3 norm, vec3 viewDir, vec3 diffTex, vec3 specTex, float shininess);


void main()
{
    MaterialGPU mat = materials[materialID];

    sampler2D diffuseSampler  = sampler2D(mat.diffuseHandle);
    sampler2D specularSampler = sampler2D(mat.specularHandle);
    sampler2D normalSampler   = sampler2D(mat.normalHandle);

    vec3  diffuseColor = mat.diffuseColorAndShininess.rgb;
    float shininess    = mat.diffuseColorAndShininess.w;

    // Diffuse
    vec4 texColor = (mat.diffuseHandle != uvec2(0)) ? texture(diffuseSampler, TexCoords) : vec4(diffuseColor, 1.0);

    // Specular
    vec3 specTex = (mat.specularHandle != uvec2(0)) ? texture(specularSampler, TexCoords).rgb : vec3(0.5);

    // Normal
    vec3 norm;
    if (mat.normalHandle != uvec2(0)) {
        vec3 n = texture(normalSampler, TexCoords).rgb * 2.0 - 1.0;
        norm = normalize(TBN * n);
    } else {
        norm = normalize(Normal);
    }

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 diffTex = texColor.rgb;
    vec3 result  = vec3(0.0);

    for (int i = 0; i < numLights; i++)
    {
        if (lights[i].params2.z < 0.5) continue; // wyłączone

        int type = int(lights[i].position.w);
        if (type == 0)
            result += CalcDirLight  (lights[i], norm, viewDir, diffTex, specTex, shininess);
        else if (type == 1)
            result += CalcPointLight (lights[i], norm, viewDir, diffTex, specTex, shininess);
        else if (type == 2)
            result += CalcSpotLight  (lights[i], norm, viewDir, diffTex, specTex, shininess);
    }

    FragColor = vec4(result, texColor.a);
}


vec3 CalcDirLight(in GPULight light, vec3 norm, vec3 viewDir, vec3 diffTex, vec3 specTex, float shininess)
{
    vec3  lightDir = normalize(-light.direction.xyz);
    float diff     = max(dot(norm, lightDir), 0.0);
    vec3  halfDir  = normalize(lightDir + viewDir);
    float spec     = pow(max(dot(norm, halfDir), 0.0), shininess);

    return light.ambient.rgb * diffTex
         + light.diffuse.rgb * diff * diffTex
         + light.specular.rgb * spec * specTex;
}

vec3 CalcPointLight(in GPULight light, vec3 norm, vec3 viewDir, vec3 diffTex, vec3 specTex, float shininess)
{
    vec3  toLight     = light.position.xyz - FragPos;
    float distance    = length(toLight);
    vec3  lightDir    = toLight / distance;
    float diff        = max(dot(norm, lightDir), 0.0);
    vec3  halfDir     = normalize(lightDir + viewDir);
    float spec        = pow(max(dot(norm, halfDir), 0.0), shininess);
    float attenuation = 1.0 / (light.params1.x + light.params1.y * distance + light.params1.z * distance * distance);

    return (light.ambient.rgb * diffTex
          + light.diffuse.rgb * diff * diffTex
          + light.specular.rgb * spec * specTex) * attenuation;
}

vec3 CalcSpotLight(in GPULight light, vec3 norm, vec3 viewDir, vec3 diffTex, vec3 specTex, float shininess)
{
    vec3  toLight     = light.position.xyz - FragPos;
    float distance    = length(toLight);
    vec3  lightDir    = toLight / distance;
    float diff        = max(dot(norm, lightDir), 0.0);
    vec3  halfDir     = normalize(lightDir + viewDir);
    float spec        = pow(max(dot(norm, halfDir), 0.0), shininess);
    float attenuation = 1.0 / (light.params1.x + light.params1.y * distance + light.params1.z * distance * distance);
    float theta       = dot(lightDir, normalize(-light.direction.xyz));
    float epsilon     = light.params2.x - light.params2.y;
    float intensity   = clamp((theta - light.params2.y) / epsilon, 0.0, 1.0);

    return (light.ambient.rgb * diffTex
          + light.diffuse.rgb * diff * diffTex
          + light.specular.rgb * spec * specTex) * attenuation * intensity;
}