#version 330

in vec3 pass_specular_colour;

out vec4 final_colour;


void main()
{
	final_colour = vec4(pass_specular_colour, 1.0);
}