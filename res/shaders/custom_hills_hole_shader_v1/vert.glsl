#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in float aAlpha;
layout(location = 3) in vec2 aUV;
layout(location = 4) in vec3 aNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 lightPos;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec3 ourColor;
out float ourAlpha;
out vec3 LightPos;
out vec3 ViewPos;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aUV;
    ourColor = aColor;
    ourAlpha = aAlpha;
    LightPos = lightPos;
    ViewPos = vec3(inverse(view) * vec4(0.0, 0.0, 0.0, 1.0));
    gl_Position = projection * view * vec4(FragPos, 1.0);
}