#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in float aAlpha;
layout(location = 3) in vec2 aTexCoord;
layout(location = 4) in vec3 aNormal;

out vec2 TexCoord;

uniform mat4 model;
uniform mat4 view_projection;

void main()
{
    // Transform the vertex position
    gl_Position = view_projection * model * vec4(aPos, 1.0);
    // Pass the texture coordinates to the fragment shader
    TexCoord = aTexCoord;
}