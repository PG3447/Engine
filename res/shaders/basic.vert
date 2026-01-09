#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

layout (location = 7) in mat4 instanceMatrix;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;


uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform bool useInstance;

void main()
{
    mat4 finalModel = useInstance ? instanceMatrix : model;

    FragPos = vec3(finalModel *  vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(finalModel))) * aNormal;
    TexCoords = aTexCoords;

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