#version 410 core

layout (location = 0) in vec3 position;

uniform mat4 uni_VP;
uniform mat4 uni_M;

out vec3 fragColor;

void main()
{
    gl_Position = uni_VP * uni_M * vec4(position, 1.0);
    fragColor = vec3(0.0, 0.0, 1.0);
}
