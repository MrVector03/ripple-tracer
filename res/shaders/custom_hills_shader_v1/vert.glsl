#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in float aAlpha;
layout(location = 3) in vec2 aTexCoord;
layout(location = 4) in vec3 aNormal;

uniform mat4 view_projection;

out vec3 ourColor;
out float ourAlpha;
out vec2 TexCoord;
out vec3 Normal;

void main() {
    gl_Position = view_projection * vec4(aPos, 1.0);
    ourColor = aColor;
    ourAlpha = aAlpha;
    TexCoord = aTexCoord;
    Normal = aNormal;
}