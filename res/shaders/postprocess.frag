#version 330 core

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D screenTexture;

void main() {
    vec3 color = texture(screenTexture, TexCoords).rgb;
    vec3 negative = 1.0 - color;   // <-- negativ
    FragColor = vec4(negative, 1.0);
}