#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;
layout (location = 5) in ivec4 boneIds;
layout (location = 6) in vec4 weights;

out vec3 FragPos;
out vec2 TexCoords;
out vec3 Normal;
out mat3 TBN;
flat out uint materialID;

// =======================
// GPU BUFFERS
// =======================

struct InstanceData {
    mat4  model;
    uint  materialID;
    uint  objectID;
    vec2  padding;
};

layout(std430, binding = 3) readonly buffer Instances
{
    InstanceData instances[];
};

uniform mat4 viewProjection;

//const int MAX_BONES = 200;
//uniform mat4 finalBonesMatrices[MAX_BONES];
//uniform bool isAnimated;

void main()
{
    // gl_BaseInstance = instanceOffset z DrawCommand (offset w instanceSSBO)
    // gl_InstanceID   = który to egzemplarz w tej instancji (0..instanceCount-1)
    InstanceData inst = instances[gl_BaseInstance + gl_InstanceID];

    materialID = inst.materialID;
    mat4 model = inst.model;

    // Skinning
//    mat4 boneTransform = mat4(1.0);
//    if (isAnimated)
//    {
//        float totalWeight = weights[0] + weights[1] + weights[2] + weights[3];
//        if (totalWeight > 0.0)
//        {
//            boneTransform  = finalBonesMatrices[boneIds[0]] * weights[0];
//            boneTransform += finalBonesMatrices[boneIds[1]] * weights[1];
//            boneTransform += finalBonesMatrices[boneIds[2]] * weights[2];
//            boneTransform += finalBonesMatrices[boneIds[3]] * weights[3];
//        }
//    }

    mat4 finalModel = model;// * boneTransform;

    vec4 worldPos = finalModel * vec4(aPos, 1.0);
    FragPos   = worldPos.xyz;
    TexCoords = aTexCoords;

    mat3 normalMatrix = mat3(transpose(inverse(finalModel)));
    vec3 T = normalize(normalMatrix * aTangent);
    vec3 B = normalize(normalMatrix * aBitangent);
    vec3 N = normalize(normalMatrix * aNormal);
    Normal = N;
    TBN    = mat3(T, B, N);

    gl_Position = viewProjection * worldPos;
}