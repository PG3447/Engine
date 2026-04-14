#version 460 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D spriteTexture;
uniform float opacity;

void main() {
    vec4 tex = texture(spriteTexture, TexCoord);
    FragColor = vec4(tex.rgb, tex.a * opacity);
}