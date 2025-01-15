#version 330 core

in vec3 ourColor;
in float ourAlpha;
in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;
in vec3 LightPos;
in vec3 ViewPos;

out vec4 FragColor;

uniform sampler2D cloudTexture;
uniform vec3 light_color;
uniform vec3 fog_color;
uniform float fog_density;
uniform float water_height;

void main() {
    // Specular lighting
    float specularStrength = 0.5;
    vec3 viewDir = normalize(ViewPos - FragPos);


    // Texture mapping
    vec2 scaledTexCoord = TexCoord;

    vec4 cloudColor = texture(cloudTexture, scaledTexCoord * 10.0);

    float height = FragPos.y;
    vec4 texColor;
    texColor = cloudColor;

    vec3 result = texColor.rgb;

    // Fog calculation
    vec3 finalColor;
    finalColor = result;
    FragColor = vec4(finalColor, texColor.a * ourAlpha);
}