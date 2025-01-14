#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in float aAlpha;
layout(location = 3) in vec2 aTexCoord;
layout(location = 4) in vec3 aNormal;

uniform mat4 view_projection;
uniform vec3 light_position;
uniform vec3 view_position;

out vec3 ourColor;
out float ourAlpha;
out vec2 TexCoord;
out vec3 Normal;
out vec3 FragPos;
out vec3 LightPos;
out vec3 ViewPos;

void main() {
    gl_Position = view_projection * vec4(aPos, 1.0);
    ourColor = aColor;
    ourAlpha = aAlpha;
    TexCoord = aTexCoord * 100.0; // Scale texture coordinates to repeat the texture
    Normal = aNormal;
    FragPos = aPos;
    LightPos = light_position;
    ViewPos = view_position;
}