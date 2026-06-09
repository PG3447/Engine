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
    uvec2 metallicRoughnessMap;
    uvec2 aoHandle;
    uvec2 normalHandle;
    uint packedColor;
    float shininess;
};


struct GPULight {
    vec4 position;  // xyz=pos,  w=type (0=dir, 1=point, 2=spot)
    vec4 direction; // xyz=dir,  w=unused
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    vec4 params1;   // x=constant, y=linear, z=quadratic, w=intensity
    vec4 params2;   // x=cutOff,   y=outerCutOff, z=enabled, w=range
};

layout(std140, binding = 0) uniform FrameUBO
{
    mat4  viewProjection;
    vec4  viewPos;   // xyz = pozycja kamery
    float zNear;
    float zFar;
    float ambientStrength;
    int numLights;
};

#define MAX_LIGHTS 512
layout(std140, binding = 1) uniform Lights
{
    GPULight lights[MAX_LIGHTS];
};

layout(std430, binding = 7) readonly restrict buffer Materials
{
    MaterialGPU materials[];
};

const float PI = 3.14159265359;

vec3 CalcDirLightPBR(in GPULight light, vec3 normal, vec3 viewDir, vec3 F0, vec3 albedo, float metallic, float roughness);
vec3 CalcPointLightPBR(in GPULight light, vec3 normal, vec3 viewDir, vec3 F0, vec3 albedo, float metallic, float roughness);
vec3 CalcSpotLightPBR(in GPULight light, vec3 normal, vec3 viewDir, vec3 F0, vec3 albedo, float metallic, float roughness);

float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 fresnelSchlick(float cosTheta, vec3 F0);

vec3 ACESFilmic(vec3 x)
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}

void main()
{
    MaterialGPU mat = materials[materialID];
    
    vec4 texColor;
    if (mat.diffuseHandle != uvec2(0))
    {
        texColor = texture(sampler2D(mat.diffuseHandle), TexCoords);
    }
    else
    {
        vec3 diffuseColor = unpackUnorm4x8(mat.packedColor).rgb;
        texColor = vec4(diffuseColor, 1.0);
    }

    vec3 albedo = pow(texColor.rgb, vec3(2.2));
    float metallic = 0.0;
    float roughness = 0.5;
    float ao = 1.0;

    if (mat.aoHandle != uvec2(0))
    {
        ao = texture(sampler2D(mat.aoHandle), TexCoords).r;
    }
    
    if (mat.metallicRoughnessMap != uvec2(0))
    {
        vec3 mr = texture(sampler2D(mat.metallicRoughnessMap), TexCoords).rgb;
        roughness = mr.g;
        metallic  = mr.b;
    
        if (mat.aoHandle == uvec2(0))
        {
            ao = mr.r;
        }
    }

    vec3 norm;
    if (mat.normalHandle != uvec2(0)) {
        vec3 n = texture(sampler2D(mat.normalHandle), TexCoords).rgb * 2.0 - 1.0;
        norm = normalize(TBN * n);
    } else {
        norm = normalize(Normal);
    }

    vec3 viewDir = normalize(viewPos.xyz - FragPos);
    

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);
    
    // równanie odbicia
    vec3 Lo = vec3(0.0);
    for (int i = 0; i < numLights; i++)
    {
        if (lights[i].params2.z < 0.5) continue; // wy³¹czone

        int type = int(lights[i].position.w);
        switch (type)
        {
            case 0:
                Lo += CalcDirLightPBR(lights[i], norm, viewDir, F0, albedo, metallic, roughness);
                break;
            case 1:
                Lo += CalcPointLightPBR(lights[i], norm, viewDir, F0, albedo, metallic, roughness);
                break;
            case 2:
                Lo += CalcSpotLightPBR(lights[i], norm, viewDir, F0, albedo, metallic, roughness);
                break;
        }
    }

    //Globalny ambient wylaczony
    vec3 ambient = vec3(ambientStrength) * albedo * ao;
    vec3 color = ambient + Lo;
    
    color = ACESFilmic(color);
    color = pow(color, vec3(1.0/2.2));  
    
    FragColor = vec4(color, 1.0);
}

vec3 CalcDirLightPBR(in GPULight light, vec3 normal, vec3 viewDir, vec3 F0, vec3 albedo, float metallic, float roughness)
{
    vec3 L = normalize(-light.direction.xyz);
    vec3 H = normalize(viewDir + L);
    vec3 radiance = light.diffuse.rgb * light.params1.w;

    float NDF = DistributionGGX(normal, H, roughness);   
    float G = GeometrySmith(normal, viewDir, L, roughness);      
    vec3 F = fresnelSchlick(max(dot(H, viewDir), 0.0), F0);
    
    vec3 numerator    = NDF * G * F; 
    float denominator = 4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
    vec3 specular = numerator / denominator;

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    // dodaj do wynikowej radiancji Lo
    float NdotL = max(dot(normal, L), 0.0);                
    return (kD * albedo / PI + specular) * radiance * NdotL; 
}

vec3 CalcPointLightPBR(in GPULight light, vec3 normal, vec3 viewDir, vec3 F0, vec3 albedo, float metallic, float roughness)
{
    vec3 L = normalize(light.position.xyz - FragPos);
    vec3 H = normalize(viewDir + L);
    float distance = length(light.position.xyz - FragPos);
    
    float attenuation = 1.0;
    float range = light.params2.w;
    if (range == 0.0f)
    {
        attenuation = 1.0 / (distance * distance);
    }
    else
    {
        attenuation = pow(clamp(1.0 - pow(distance / range, 4.0), 0.0, 1.0), 2.0) / (distance * distance + 1.0);
    }
    vec3 radiance = light.diffuse.rgb * light.params1.w * attenuation;
                
    
    float NDF = DistributionGGX(normal, H, roughness);   
    float G = GeometrySmith(normal, viewDir, L, roughness);      
    vec3 F = fresnelSchlick(max(dot(H, viewDir), 0.0), F0);
    
    vec3 numerator    = NDF * G * F; 
    float denominator = 4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
    vec3 specular = numerator / denominator;

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    // dodaj do wynikowej radiancji Lo
    float NdotL = max(dot(normal, L), 0.0);                
    return (kD * albedo / PI + specular) * radiance * NdotL; 
}

vec3 CalcSpotLightPBR(in GPULight light, vec3 normal, vec3 viewDir, vec3 F0, vec3 albedo, float metallic, float roughness)
{
    vec3 toLight = light.position.xyz - FragPos;
    float distance = length(toLight);
    vec3 L = toLight / distance;
    vec3 H = normalize(viewDir + L);
    
    //float attenuation = 1.0 / (lights[i].params1.x + lights[i].params1.y * distance + lights[i].params1.z * distance * distance);
    float attenuation = 1.0;
    float range = light.params2.w;
    if (range == 0.0f)
    {
        attenuation = 1.0 / (distance * distance);
    }
    else
    {
        attenuation = pow(clamp(1.0 - pow(distance / range, 4.0), 0.0, 1.0), 2.0) / (distance * distance + 1.0);
    }
    // sto¿ek spotlighta – identyczny jak w Blinn-Phong
    float theta = dot(L, normalize(-light.direction.xyz));
    float intensity = smoothstep(light.params2.y, light.params2.x, theta);
    //float epsilon   = light.params2.x - light.params2.y;
    //float intensity = clamp((theta - light.params2.y) / epsilon, 0.0, 1.0);
    
    vec3 radiance = light.diffuse.rgb * light.params1.w * attenuation * intensity;
    
    float NDF = DistributionGGX(normal, H, roughness);   
    float G = GeometrySmith(normal, viewDir, L, roughness);      
    vec3 F = fresnelSchlick(max(dot(H, viewDir), 0.0), F0);
    
    vec3 numerator    = NDF * G * F; 
    float denominator = 4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
    vec3 specular = numerator / denominator;

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    // dodaj do wynikowej radiancji Lo
    float NdotL = max(dot(normal, L), 0.0);                
    return (kD * albedo / PI + specular) * radiance * NdotL; 
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}


/*

        switch (type)
        {
            case 0:
                L = normalize(-lights[i].direction.xyz);
                H = normalize(viewDir + L);
                radiance = lights[i].diffuse.rgb * lights[i].params1.w;
                //result += CalcDirLight(lights[i], norm, viewDir, diffTex, specTex, shininess);
                break;
            case 1: // point light
                // obliczy radiancjê per-œwiat³o
                L = normalize(lights[i].position.xyz - FragPos);
                H = normalize(viewDir + L);
                distance = length(lights[i].position.xyz - FragPos);

                range = lights[i].params2.w;
                if (range == 0.0f)
                {
                    attenuation = 1.0 / (distance * distance);
                }
                else
                {
                    attenuation = pow(clamp(1.0 - pow(distance / range, 4.0), 0.0, 1.0), 2.0) / (distance * distance + 1.0);
                }
                radiance = lights[i].diffuse.rgb * lights[i].params1.w * attenuation;
                
                break;
            case 2:
                vec3  toLight = lights[i].position.xyz - FragPos;
                distance = length(toLight);
                L = toLight / distance;
                H = normalize(viewDir + L);

                //float attenuation = 1.0 / (lights[i].params1.x + lights[i].params1.y * distance + lights[i].params1.z * distance * distance);
                range = lights[i].params2.w;
                if (range == 0.0f)
                {
                    attenuation = 1.0 / (distance * distance);
                }
                else
                {
                    attenuation = pow(clamp(1.0 - pow(distance / range, 4.0), 0.0, 1.0), 2.0) / (distance * distance + 1.0);
                }
                // sto¿ek spotlighta – identyczny jak w Blinn-Phong
                float theta = dot(L, normalize(-lights[i].direction.xyz));
                float epsilon   = lights[i].params2.x - lights[i].params2.y;
                float intensity = clamp((theta - lights[i].params2.y) / epsilon, 0.0, 1.0);

                radiance = lights[i].diffuse.rgb * lights[i].params1.w * attenuation * intensity;

                //result += CalcSpotLight(lights[i], norm, FragPos, viewDir, diffTex, specTex, shininess);
                break;
        }
    
        // cook-torrance brdf
        float NDF = DistributionGGX(norm, H, roughness);   
        float G   = GeometrySmith(norm, viewDir, L, roughness);      
        vec3 F    = fresnelSchlick(max(dot(H, viewDir), 0.0), F0);
        
        vec3 numerator    = NDF * G * F; 
        float denominator = 4.0 * max(dot(norm, viewDir), 0.0) * max(dot(norm, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
        vec3 specular = numerator / denominator;

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;
    
        // dodaj do wynikowej radiancji Lo
        float NdotL = max(dot(norm, L), 0.0);                
        Lo += (kD * albedo / PI + specular) * radiance * NdotL; 
*/