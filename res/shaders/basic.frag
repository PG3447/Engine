#version 460 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D texture_diffuse1;

void main()
{    
    FragColor = texture(texture_diffuse1, TexCoords);
}

//#version 460 core
//out vec4 color;
//in vec2 ourTexCoord;
//
//uniform vec4 fractalColor;
//uniform sampler2D ourTexture;
//
//void main()
//{
//	color = texture(ourTexture, ourTexCoord) * fractalColor;
//}