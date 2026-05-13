#version 330 core

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D screenTexture;
uniform float gamma;

void main() {
    //vec2 uv = TexCoords;
    //uv.y += sin(uv.x * 20.0) * 0.02;
    //vec3 color = texture(screenTexture, uv).rgb;

    vec3 color = texture(screenTexture, TexCoords).rgb;
    //vec3 negative = 1.0 - color;
    //FragColor = vec4(negative, 1.0);

    vec3 playerColor;

    //color efects
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
        float R = color.r;
        float G = color.g;
        float B = color.b;

        float s1 = R - G;
        float s2 = R - B;
        float s3 = B - R;
        float s4 = B - G;

        float m1 = max(s1, s2);
        float m2 = max(s3, s4);

        float ss1 = smoothstep(0.08, 0.15, m1);
        float ss2 = smoothstep(0.08, 0.15, m2);

        float GIGAWYNIK = max(m1, m2);

        float dotProduct = dot(color, vec3(0.25, 0.0, 0.11));

        vec3 tempColor =  mix(vec3(dotProduct) ,color, GIGAWYNIK);

        playerColor = pow(tempColor, vec3(1.0 / 1.8));//dont worry about it
    }
    //color efects

    //Efekt 1
    float scanline = sin(TexCoords.y * 800.0) * 0.04;
    playerColor -= scanline;

    //Efekt 2
    float noise = fract(sin(dot(TexCoords, vec2(12.9898,78.233))) * 43758.5453);
    playerColor += noise * 0.05;

    //Contrast
    float contrast = 1.5;
    playerColor = (playerColor - 0.5) * contrast + 0.5;

    //Efekt 3
    vec2 uv = TexCoords;
    float dist = distance(uv, vec2(0.5));
    float vignette = smoothstep(0.8, 0.3, dist);

    playerColor *= vignette;


    vec3 finalColor = pow(playerColor, vec3(1.0 / gamma));//gamma

    FragColor = vec4(finalColor, 1.0);
}