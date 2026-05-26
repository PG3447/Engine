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
    mat4 model;
    uint materialID;
    uint objectID;
    uint skeletonID; 
    uint padding;
};

// Jedna p³aska tablica wszystkich macierzy koœci dla WSZYSTKICH szkieletów.
// Uk³ad: skeleton 0 zajmuje [0 .. MAX_BONES-1],
//        skeleton 1 zajmuje [MAX_BONES .. 2*MAX_BONES-1], itd.
const uint MAX_BONES = 200u;

layout(std430, binding = 3) readonly buffer Instances
{
    InstanceData instances[];
};

layout(std430, binding = 4) readonly buffer BoneMatrices
{
    mat4 boneMatrices[]; //rozmiar MAX_BONES * maxSkeletons
};

uniform mat4 viewProjection;

mat3 cofactorMatrix(mat4 m)
{
    vec3 c0 = m[0].xyz;
    vec3 c1 = m[1].xyz;
    vec3 c2 = m[2].xyz;
    return mat3(
        cross(c1, c2),
        cross(c2, c0),
        cross(c0, c1)
    );
}

void main()
{
    // gl_BaseInstance = instanceOffset z DrawCommand (offset w instanceSSBO)
    // gl_InstanceID   = który to egzemplarz w tej instancji (0..instanceCount-1)
    InstanceData inst = instances[gl_BaseInstance + gl_InstanceID];

    materialID = inst.materialID;
    mat4 model = inst.model;

    // Skinning
    mat4 boneTransform = mat4(1.0);

    if (inst.skeletonID != 0xFFFFFFFFu)
    {
        float totalWeight = weights[0] + weights[1] + weights[2] + weights[3];
        if (totalWeight > 0.0)
        {
            uint base = inst.skeletonID * MAX_BONES; // offset w BoneMatrices[]

            boneTransform  = boneMatrices[base + uint(boneIds[0])] * weights[0];
            boneTransform += boneMatrices[base + uint(boneIds[1])] * weights[1];
            boneTransform += boneMatrices[base + uint(boneIds[2])] * weights[2];
            boneTransform += boneMatrices[base + uint(boneIds[3])] * weights[3];
        }
    }

    mat4 finalModel = model * boneTransform;

    vec4 worldPos = finalModel * vec4(aPos, 1.0);
    FragPos   = worldPos.xyz;
    TexCoords = aTexCoords;

    mat3 normalMatrix = mat3(transpose(inverse(finalModel))); // cofactorMatrix(finalModel);
    vec3 T = normalize(normalMatrix * aTangent);
    vec3 B = normalize(normalMatrix * aBitangent);
    vec3 N = normalize(normalMatrix * aNormal);
    Normal = N;
    TBN    = mat3(T, B, N);

    gl_Position = viewProjection * worldPos;
}