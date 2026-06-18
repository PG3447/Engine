#version 460 core
#extension GL_ARB_bindless_texture : require
#define MAX_SHADOW_LIGHTS 32

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
    vec4 direction; // xyz=dir,  w=shadowON-shadowOFF
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    vec4 params1;   // x=constant, y=linear, z=quadratic, w=intensity
    vec4 params2;   // x=cutOff,   y=outerCutOff, z=enabled, w=range
};

//uniform sampler2DArrayShadow shadowMap;
uniform sampler2DArray shadowMap;

layout(std140, binding = 0) uniform FrameUBO
{
    mat4 viewProjection;
    vec4 viewPos;   // xyz = pozycja kamery
    float ambientStrength;
    int numLights;
    int numShadowLigths;
    int padding;
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

layout(std430, binding = 8) readonly buffer ShadowMatrices
{
    mat4 lightSpaceMatrices[]; // tylko światła z castShadows, max MAX_SHADOW_LIGHTS
};

const float PI = 3.14159265359;

vec3 CalcDirLightPBR(in GPULight light, vec3 normal, vec3 viewDir, vec3 F0, vec3 albedo, float metallic, float roughness);
vec3 CalcPointLightPBR(in GPULight light, vec3 normal, vec3 viewDir, vec3 F0, vec3 albedo, float metallic, float roughness);
vec3 CalcSpotLightPBR(in GPULight light, vec3 normal, vec3 viewDir, vec3 F0, vec3 albedo, float metallic, float roughness, int layer);

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


//float ShadowCalculation(vec4 fragPosLightSpace, vec3 norm, vec3 lightDir, int layer)
//{
//    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
//    projCoords = projCoords * 0.5 + 0.5;
//
//    // poza frustumem światła = brak cienia
//    if (projCoords.z > 1.0 || any(lessThan(projCoords.xy, vec2(0.0))) || any(greaterThan(projCoords.xy, vec2(1.0))))
//        return 0.0;
//
//    float currentDepth = projCoords.z;
//
//    // bias zależny od kąta — znacznie mniejsze wartości
//    float cosTheta = max(dot(normalize(norm), normalize(lightDir)), 0.0);
//    float bias = mix(0.0005, 0.00005, cosTheta);
//
//    float shadow = 0.0;
//    vec2 texelSize = 1.0 / textureSize(shadowMap, 0).xy;
//    for (int x = -1; x <= 1; ++x)
//        for (int y = -1; y <= 1; ++y)
//            shadow += texture(shadowMap,
//                vec4(projCoords.xy + vec2(x, y) * texelSize,
//                     float(layer),
//                     currentDepth - bias));
//
//    return 1.0 - (shadow / 9.0);
//}

float ShadowCalculation(vec4 fragPosLightSpace, vec3 norm, vec3 lightPos, int layer)
{
     // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    //float closestDepth = texture(shadowMap, projCoords.xy).r;
 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // calculate bias (based on depth map resolution and slope)
    vec3 normal = normalize(norm);
    vec3 lightDir = normalize(lightPos - FragPos);
    float bias = max(0.001 * (1.0 - dot(normal, lightDir)), 0.0005);
    // check whether current frag pos is in shadow
    // float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;
    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0).xy;
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            //float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            //float pcfDepth = texture(shadowMap, vec4(projCoords.xy +  vec2(x, y) * texelSize, float(layer), currentDepth - bias));
            //shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;        
            //shadow += texture(shadowMap, vec4(projCoords.xy +  vec2(x, y) * texelSize, float(layer), currentDepth - bias)); 
            float pcfDepth = texture(shadowMap, vec3(projCoords.xy + vec2(x, y) * texelSize, float(layer))).r;
            shadow += (currentDepth - bias > pcfDepth) ? 1.0 : 0.0;
        }    
    }
    shadow /= 9.0;
    
    // keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if(projCoords.z > 1.0)
        shadow = 0.0;
    return shadow;
}


//float ShadowCalculation(
//    vec4 fragPosLightSpace,
//    vec3 norm,
//    vec3 lightPos,
//    int layer)
//{
//    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
//    projCoords = projCoords * 0.5 + 0.5;
//
//    if (projCoords.z > 1.0 ||
//        projCoords.x < 0.0 || projCoords.x > 1.0 ||
//        projCoords.y < 0.0 || projCoords.y > 1.0)
//    {
//        return 0.0;
//    }
//
//    float currentDepth = projCoords.z;
//
//    vec3 normal = normalize(norm);
//    vec3 lightDir = normalize(lightPos - FragPos);
//
//    float bias = max(
//        0.005 * (1.0 - dot(normal, lightDir)),
//        0.0005
//    );
//
//    float shadow = 0.0;
//
//    vec2 texelSize =
//        1.0 / vec2(textureSize(shadowMap, 0).xy);
//
//    for (int x = -1; x <= 1; ++x)
//    {
//        for (int y = -1; y <= 1; ++y)
//        {
//            float pcfDepth =
//                texture(
//                    shadowMap,
//                    vec3(
//                        projCoords.xy + vec2(x, y) * texelSize,
//                        float(layer)
//                    )
//                ).r;
//
//            shadow +=
//                (currentDepth - bias > pcfDepth)
//                ? 1.0
//                : 0.0;
//        }
//    }
//
//    shadow /= 9.0;
//
//    return shadow;
//}

void main()
{
    MaterialGPU mat = materials[materialID];
    
    vec4 texColor;
    if (mat.diffuseHandle != uvec2(0))
    {
        texColor = texture(sampler2D(mat.diffuseHandle), TexCoords);

//        float opacity = unpackUnorm4x8(mat.packedColor).a;
//
//        if (opacity < 1.0f)
//        {
//            texColor.a = opacity;
//        }
    }
    else
    {
        texColor = unpackUnorm4x8(mat.packedColor);
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
        if (lights[i].params2.z < 0.5) continue; // wyłączone

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
                Lo += CalcSpotLightPBR(lights[i], norm, viewDir, F0, albedo, metallic, roughness, i);
                break;
        }
    }

    //Globalny ambient wylaczony
    vec3 ambient = vec3(ambientStrength) * albedo * ao;
    vec3 color = ambient + Lo;
    
    color = ACESFilmic(color);
    color = pow(color, vec3(1.0/2.2));  
    
    FragColor = vec4(color, texColor.a);
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

    if (range >= 10000.0f)
    {
        attenuation = clamp(1.0 - distance / (range/10000.0f ), 0.0, 1.0);
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

vec3 CalcSpotLightPBR(in GPULight light, vec3 normal, vec3 viewDir, vec3 F0, vec3 albedo, float metallic, float roughness, int layer)
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

    if (range >= 10000.0f)
    {
        attenuation = clamp(1.0 - distance / (range/10000.0f ), 0.0, 1.0);
    }

    // stożek spotlighta – identyczny jak w Blinn-Phong
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
    
    
    float shadow = 0.0f;// ShadowCalculation();

    vec4 fragPosLS = lightSpaceMatrices[layer] * vec4(FragPos, 1.0);

//return vec3(
//    fragPosLS.xyz / fragPosLS.w * 0.5 + 0.5
//);
        shadow = ShadowCalculation(fragPosLS, normal, light.position.xyz, layer);
    

    // dodaj do wynikowej radiancji Lo
    float NdotL = max(dot(normal, L), 0.0);                
    return (kD * albedo / PI + specular) * radiance * NdotL * (1.0 - shadow); 
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
                // obliczy radiancję per-światło
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
                // stożek spotlighta – identyczny jak w Blinn-Phong
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