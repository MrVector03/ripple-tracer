#version 410 core

layout (location = 0) in vec3 position;

uniform mat4 uni_VP;
uniform mat4 uni_M;

out vec3 fragColor;

uniform vec4 plane;

void main()
{
    gl_Position = uni_VP * uni_M * vec4(position, 1.0);
    fragColor = vec3(0.0, 0.0, 1.0);

    gl_ClipDistance[0] = dot(gl_Position, plane);
}
