#version 460 core

layout(points) in;
layout(triangle_strip, max_vertices = 150) out; // zwiększamy max_vertices

flat in int vRingID[]; // odbieramy ringID z VS

out vec3 Normal;
out vec2 Tex;

uniform float radius;
uniform int rings;
uniform int sectors;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Funkcja emitująca jeden wierzchołek z atrybutami
void EmitVertexWithAttributes(vec3 pos, int r, int s)
{
    Normal = normalize(pos);
    Tex = vec2(float(s)/float(sectors), float(r)/float(rings));
    gl_Position = projection * view * model * vec4(pos, 1.0);
    EmitVertex();
}

void main()
{
    int r = vRingID[0]; // ID ringu, który generujemy w tym punkcie

    float pi = 3.14159265359;

    float thetaR = pi * float(r) / float(rings);
    float thetaNextR = pi * float(r + 1) / float(rings);

    for (int s = 0; s < sectors; ++s)
    {
        int nextS = (s + 1) % sectors;
        float phiS = 2.0 * pi * float(s) / float(sectors);
        float phiNextS = 2.0 * pi * float(nextS) / float(sectors);

        vec3 pos00 = vec3(radius * sin(thetaR) * cos(phiS),
                          radius * cos(thetaR),
                          radius * sin(thetaR) * sin(phiS));

        vec3 pos01 = vec3(radius * sin(thetaR) * cos(phiNextS),
                          radius * cos(thetaR),
                          radius * sin(thetaR) * sin(phiNextS));

        vec3 pos10 = vec3(radius * sin(thetaNextR) * cos(phiS),
                          radius * cos(thetaNextR),
                          radius * sin(thetaNextR) * sin(phiS));

        vec3 pos11 = vec3(radius * sin(thetaNextR) * cos(phiNextS),
                          radius * cos(thetaNextR),
                          radius * sin(thetaNextR) * sin(phiNextS));

        // dwa trójkąty dla patcha
        EmitVertexWithAttributes(pos00, r, s);
        EmitVertexWithAttributes(pos10, r + 1, s);
        EmitVertexWithAttributes(pos11, r + 1, nextS);
        EndPrimitive();

        EmitVertexWithAttributes(pos00, r, s);
        EmitVertexWithAttributes(pos11, r + 1, nextS);
        EmitVertexWithAttributes(pos01, r, nextS);
        EndPrimitive();
    }
}
