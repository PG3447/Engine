#version 460 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D spriteTexture;
uniform float opacity;
uniform vec2 scrollOffset;

void main() {
    vec2 uv = TexCoord + scrollOffset;
    vec4 tex = texture(spriteTexture, uv);
    if (tex.a < 0.01) discard;
    FragColor = vec4(tex.rgb, tex.a * opacity);
}