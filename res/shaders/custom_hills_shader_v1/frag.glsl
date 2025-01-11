#version 330 core

in vec3 ourColor;
in float ourAlpha;
in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;
in vec3 LightPos;
in vec3 ViewPos;

out vec4 FragColor;

uniform sampler2D hillTexture;
uniform vec3 light_color;

void main() {
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * light_color;

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(LightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * light_color;

    float specularStrength = 0.5;
    vec3 viewDir = normalize(ViewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * light_color;

    vec3 result = (ambient + diffuse + specular) * ourColor;
    vec4 texColor = texture(hillTexture, TexCoord);
    FragColor = texColor * vec4(result, ourAlpha);
}