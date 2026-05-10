#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

layout (location = 5) in ivec4 boneIds;
layout (location = 6) in vec4 weights;

layout (location = 7) in mat4 instanceMatrix;

out vec3 FragPos;
out vec2 TexCoords;
out vec3 Normal;
out mat3 TBN;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform bool useInstance;

const int MAX_BONES = 200;
uniform mat4 finalBonesMatrices[MAX_BONES];
uniform bool isAnimated;

void main()
{
    mat4 boneTransform = mat4(1.0);
    
    if(isAnimated)
    {
        float totalWeight = weights[0] + weights[1] + weights[2] + weights[3];

        if (totalWeight > 0.0) 
        {
            boneTransform =  finalBonesMatrices[boneIds[0]] * weights[0];
            boneTransform += finalBonesMatrices[boneIds[1]] * weights[1];
            boneTransform += finalBonesMatrices[boneIds[2]] * weights[2];
            boneTransform += finalBonesMatrices[boneIds[3]] * weights[3];
        }
    }

    mat4 finalModel = useInstance ? instanceMatrix : model;
    
    mat4 modelWithSkinning = finalModel * boneTransform;

    FragPos = vec3(modelWithSkinning * vec4(aPos, 1.0));
    TexCoords = aTexCoords;

    mat3 normalMatrix = mat3(transpose(inverse(modelWithSkinning)));
    Normal = normalize(normalMatrix * aNormal);

    vec3 T = normalize(normalMatrix * aTangent);
    vec3 B = normalize(normalMatrix * aBitangent);
    vec3 N = normalize(normalMatrix * aNormal);
    TBN = mat3(T, B, N);

    gl_Position = projection * view * vec4(FragPos, 1.0);
}