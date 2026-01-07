#version 460 core

in vec3 Normal;
in vec2 Tex;

out vec4 FragColor;

uniform sampler2D texture_diffuse1; // np. Twoja tekstura

void main()
{    
    FragColor = texture(texture_diffuse1, Tex);
}