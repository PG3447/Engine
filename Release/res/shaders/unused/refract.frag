#version 460
out vec4 FragColor;

in vec3 Normal;
in vec3 Position;

uniform vec3 cameraPos;
uniform samplerCube skybox;

void main()
{
    vec3 ior = vec3(1.51, 1.52, 1.53);
    vec3 I = normalize(Position - cameraPos);

    vec3 Rr = refract(I, normalize(Normal), 1.0 / ior.r);
    vec3 Rg = refract(I, normalize(Normal), 1.0 / ior.g);
    vec3 Rb = refract(I, normalize(Normal), 1.0 / ior.b);

    vec3 color;
    color.r = texture(skybox, Rr).r;
    color.g = texture(skybox, Rg).g;
    color.b = texture(skybox, Rb).b;

    FragColor = vec4(color, 1.0);
}