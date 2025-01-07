#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;

uniform mat4 uni_M;
uniform mat4 uni_VP;

void main()
{
    FragPos = vec3(uni_M * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(uni_M))) * aNormal;
    gl_Position = uni_VP * vec4(FragPos, 1.0);
}