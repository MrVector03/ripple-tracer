#version 330

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 colour;
layout (location = 2) in float alpha;
layout (location = 3) in vec2 uv;
layout (location = 4) in vec3 normal;

out vec3 pass_colour;
out vec2 pass_uv;
out vec3 pass_normal;

out vec3 pass_world_position;
out vec3 LightPos;

uniform mat4 uni_M;
uniform mat4 uni_VP;
uniform vec3 light_position;

void main()
{
    vec4 world_position = uni_M * vec4(position, 1.0);

    pass_world_position = world_position.xyz;

    gl_Position = uni_VP * world_position;

    pass_colour = colour;
    pass_uv = uv;
    pass_normal = (uni_M * vec4(normal, 0.0)).xyz;

    LightPos = light_position;
}