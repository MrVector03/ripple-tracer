#version 330

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 uv;

out vec2 frag_uv;
out vec3 frag_position;

uniform mat4 uni_M;
uniform mat4 uni_VP;

void main()
{
    frag_uv = uv;
    frag_position = position;
    gl_Position = uni_VP * uni_M * vec4(position, 1.0);
}