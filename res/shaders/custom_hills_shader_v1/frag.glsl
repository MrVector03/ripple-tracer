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
uniform vec3 fog_color;
uniform float fog_density;

void main() {
    // Lighting calculations
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

    vec3 lighting = ambient + diffuse + specular;

    // Get the texture color
    vec4 texColor = texture(hillTexture, TexCoord);

    // Combine texture color with lighting
    vec3 result = texColor.rgb * lighting;

    // Fog calculation
    float distance = length(FragPos - ViewPos);
    float fog_factor = exp(-fog_density * distance * distance); // Exponential fog
    fog_factor = clamp(fog_factor, 0.0, 1.0);

    // Mix the fog color (sky color) with the final lit texture
    vec3 finalColor = mix(fog_color, result, fog_factor);

    // Output the final fragment color
    FragColor = vec4(finalColor, texColor.a * ourAlpha);
}