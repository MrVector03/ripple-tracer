#version 330 core

in vec3 fragColor;
in float height;

out vec4 FragColor;

void main()
{
    vec3 lowColor = vec3(0.0, 0.8, 0.0); // green
    vec3 midColor = vec3(0.8, 0.8, 0.0); // yellow
    vec3 highColor = vec3(1.0, 1.0, 1.0); // white

    float normalizedHeight = clamp(height / 10.0, 0.0, 1.0);

    vec3 color;
    if (normalizedHeight < 0.5)
    {
        color = mix(lowColor, midColor, normalizedHeight * 2.0);
    }
    else
    {
        color = mix(midColor, highColor, (normalizedHeight - 0.5) * 2.0);
    }

    FragColor = vec4(color, 1.0);
}
