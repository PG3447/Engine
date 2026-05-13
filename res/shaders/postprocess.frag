#version 330 core

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D screenTexture;
uniform float gamma;
uniform float time;

void main() {
    //vec2 uv = TexCoords;
    //uv.y += sin(uv.x * 20.0 + time) * 0.02;
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

        float dotProduct = dot(color, vec3(0.0, 0.587, 0.114));

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

        float dotProduct = dot(color, vec3(0.299, 0.0, 0.114));

        vec3 tempColor =  mix(vec3(dotProduct) ,color, GIGAWYNIK);

        playerColor = pow(tempColor, vec3(1.0 / 1.8));//dont worry about it
    }

    //Contrast
    float contrast = 1.5;
    playerColor = (playerColor - 0.5) * contrast + 0.5;

    //Lines
    float scanline = sin(TexCoords.y * 800.0) * 0.04;
    playerColor -= scanline;

    //Grain
    float noise = fract(sin(dot(TexCoords + time * 0.1, vec2(12.9898,78.233))) * 43758.5453);
    playerColor += noise * 0.1;

    playerColor = pow(playerColor, vec3(1.0 / gamma));//gamma

    //Vignette
    vec2 center;
    if(TexCoords.x < 0.5)
    {
        center = vec2(0.25, 0.5);
    }
    else
    {
        center = vec2(0.75, 0.5);
    }
    float dist = distance(TexCoords, center);
    float vignette = smoothstep(0.7, 0.2, dist);
    playerColor *= vignette;

    //divider
    float divider = smoothstep(0.001, 0.005, abs(TexCoords.x - 0.5));
    playerColor *= divider;

    FragColor = vec4(playerColor, 1.0);

}