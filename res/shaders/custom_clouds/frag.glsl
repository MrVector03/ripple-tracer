#version 330 core

in vec2 TexCoord;

out vec4 FragColor;

void main()
{
    // Output a red color
    FragColor = vec4(1.0, 0.0, 0.0, 1.0); // Red color with full opacity
}