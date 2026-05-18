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

layout(std430, binding = 7) readonly buffer Materials
{
    MaterialGPU materials[];
};

void main()
{
    MaterialGPU mat = materials[materialID];

    // Odtwórz sampler z bindless handle
    sampler2D diffuseSampler  = sampler2D(mat.diffuseHandle);
    sampler2D normalSampler   = sampler2D(mat.normalHandle);

    vec3 diffuseColor = mat.diffuseColorAndShininess.rgb;
    float shininess    = mat.diffuseColorAndShininess.w;

    vec4 texColor;
    if (mat.diffuseHandle != uvec2(0))
        texColor = texture(diffuseSampler, TexCoords);
    else
        texColor = vec4(diffuseColor, 1.0);

    // Normal map
    vec3 norm;
    if (mat.normalHandle != uvec2(0)) {
        vec3 n = texture(normalSampler, TexCoords).rgb * 2.0 - 1.0;
        norm = normalize(TBN * n);
    } else {
        norm = normalize(Normal);
    }

    // Proste oœwietlenie kierunkowe ¿eby coœ by³o widaæ
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 color = texColor.rgb * (0.2 + 0.8 * diff);

    FragColor = vec4(color, texColor.a);
}