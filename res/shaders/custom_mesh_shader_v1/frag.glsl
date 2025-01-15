#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 light_pos;
uniform vec3 light_color;
uniform vec3 view_pos;
uniform vec3 object_color;
uniform samplerCube environmentMap;

void main()
{
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * light_color;

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light_pos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * light_color;

    float specularStrength = 1.0;
    vec3 viewDir = normalize(view_pos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * light_color;

    vec3 reflected = reflect(-viewDir, norm);
    vec3 envColor = texture(environmentMap, reflected).rgb;

    vec3 result = (ambient + diffuse + specular) * object_color;

    vec3 finalColor = mix(result, envColor, 0.5);

    FragColor = vec4(finalColor, 1.0);
}