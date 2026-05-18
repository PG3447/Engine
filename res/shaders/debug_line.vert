#version 460 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aColor;

uniform mat4 uVP;
uniform vec4 uColor;
uniform bool uUseUniformColor;

out vec4 vColor;

void main() {
    vColor = uUseUniformColor ? uColor : aColor;
    gl_Position = uVP * vec4(aPos, 1.0);
}