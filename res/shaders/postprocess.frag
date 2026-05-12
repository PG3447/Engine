#version 330 core

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D screenTexture;
uniform float gamma;

void main() {
    vec3 color = texture(screenTexture, TexCoords).rgb;
    //vec3 negative = 1.0 - color;
    //FragColor = vec4(negative, 1.0);

    vec3 playerColor;

    //green camera
    if(TexCoords.x < 0.5){
        float R = color.r;
        float G = color.g;
        float B = color.b;

        float s1 = G - R;
        float s2 = G - B;
        float s3 = B - R;
        float s4 = B - G;

        float m1 = max(s1, s2);
        float m2 = max(s3, s4);

        float ss1 = smoothstep(0.08, 0.15, m1);
        float ss2 = smoothstep(0.08, 0.15, m2);

        float GIGAWYNIK = max(m1, m2);

        float dotProduct = dot(color, vec3(0.0, 0.58, 0.11));

        playerColor =  mix(vec3(dotProduct) ,color, GIGAWYNIK);
    } else {
        playerColor =  color;
    }
    //green camera end

    vec3 finalColor = pow(playerColor, vec3(1.0 / gamma));//gamma

    FragColor = vec4(finalColor, 1.0);
}