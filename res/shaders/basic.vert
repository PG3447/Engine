#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

layout (location = 7) in mat4 instanceMatrix;

out vec3 FragPos;
out vec2 TexCoords;
out vec3 Normal;
out mat3 TBN;


uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform bool useInstance;

void main()
{
    mat4 finalModel = useInstance ? instanceMatrix : model;

    FragPos = vec3(finalModel * vec4(aPos, 1.0));
    TexCoords = aTexCoords;

    mat3 normalMatrix = mat3(transpose(inverse(finalModel)));
    
    Normal = normalMatrix * aNormal;

    vec3 T = normalize(normalMatrix * aTangent);
    vec3 B = normalize(normalMatrix * aBitangent);
    vec3 N = normalize(normalMatrix * aNormal);
    TBN = mat3(T, B, N);

    gl_Position = projection * view * finalModel * vec4(aPos, 1.0);
}


//#version 460 core
//layout (location = 0) in vec3 position;
//layout (location = 1) in vec2 texCoord;
//
//out vec2 ourTexCoord;
//
//uniform mat4 model;
//uniform mat4 view;
//uniform mat4 projection;
//
//void main()
//{
//	gl_Position = projection * view * model * vec4(position, 1.0f);  
//    ourTexCoord = texCoord;
//}