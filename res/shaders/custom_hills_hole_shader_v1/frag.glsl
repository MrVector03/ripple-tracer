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
uniform sampler2D sandTexture;
uniform sampler2D grassTexture;
uniform vec3 light_color;
uniform vec3 fog_color;
uniform float fog_density;
uniform float water_height;
uniform vec3 cloud_light_position;
uniform vec3 cloud_light_color;
uniform sampler2D cloudShadowMap;
uniform mat4 lightSpaceMatrix;

vec3 calculateShadow(vec4 fragPosLightSpace) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    float closestDepth = texture(cloudShadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;

    float shadow = currentDepth > closestDepth + 0.005 ? 0.5 : 1.0;

    return vec3(shadow);
}

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

    vec4 fragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);
    vec3 shadow = calculateShadow(fragPosLightSpace);
    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * cloud_light_color;

    vec4 sandColor = texture(sandTexture, TexCoord * 10.0);
    vec4 hillColor = texture(hillTexture, TexCoord * 10.0);
    vec4 grassColor = texture(grassTexture, TexCoord * 10.0);

    float height = FragPos.y;
    vec4 texColor;
    if (height < water_height) {
        texColor = sandColor;
    } else if (height < water_height + 4.0) {
        float t = (height - water_height) / 4.0;
        texColor = mix(sandColor, hillColor, t);
    } else if (height < water_height + 8.0) {
        float t = (height - (water_height + 4.0)) / 4.0;
        texColor = mix(hillColor, grassColor, t);
    } else {
        texColor = grassColor;
    }

    vec3 result = texColor.rgb * lighting;

    float distance = length(FragPos - ViewPos);
    float fog_factor = exp(-fog_density * distance * distance);
    fog_factor = clamp(fog_factor, 0.0, 1.0);

    vec3 finalColor = mix(fog_color, result, fog_factor);

    FragColor = vec4(finalColor, texColor.a * ourAlpha);
}