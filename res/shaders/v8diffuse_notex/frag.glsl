#version 330

in vec3 pass_normal;

out vec4 final_colour;

uniform vec3 uni_object_colour;
uniform vec3 uni_light_colour;
uniform vec3 uni_light_direction;
uniform vec3 uni_ambient;


void main()
{
	float light_factor = clamp(dot(normalize(pass_normal), normalize(uni_light_direction * (-1))), 0, 1);
	vec3 fcolour = (uni_ambient * uni_object_colour) + (uni_object_colour * (uni_light_colour * light_factor));
	//fcolour = (pass_normal + vec3(1.0)) * 0.5;
	final_colour = vec4(fcolour, 1.0f);
}