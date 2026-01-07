#version 460 core

layout(location = 0) in float ringID; // ID ringu

flat out int vRingID; // przekazanie do GS

void main()
{
    vRingID = int(ringID);  // tylko przekazujemy ID ringu
    gl_Position = vec4(0.0); // punkt jest niewidoczny
}
