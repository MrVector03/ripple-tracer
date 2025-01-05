#version 330

in vec3 pass_specular_colour;
in vec3 pass_diffuse_colour;

out vec4 final_colour;


void main()
{
	final_colour = vec4(pass_specular_colour + pass_diffuse_colour, 1.0);
}