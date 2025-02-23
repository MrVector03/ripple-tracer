#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 uv;

out vec2 pass_uv;
out vec3 pass_world_position;

uniform mat4 uni_M;
uniform mat4 uni_VP;

void main()
{
    vec4 world_position = uni_M * vec4(position, 1.0);
    pass_world_position = world_position.xyz;
    pass_uv = uv;

    gl_Position = uni_VP * world_position;
}