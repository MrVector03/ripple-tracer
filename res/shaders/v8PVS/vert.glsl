#version 330

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 uv;
layout (location = 2) in vec3 normal;

out vec3 pass_specular_colour;

uniform mat4 uni_M;
uniform mat4 uni_VP;

uniform vec3 uni_object_colour;
uniform vec3 uni_light_colour;
uniform vec3 uni_light_direction;
uniform vec3 uni_ambient;
uniform vec3 uni_camera_position;

void main()
{
	vec4 world_position = uni_M * vec4(position, 1.0);	
	

	
	gl_Position = uni_VP * world_position;
	
	
	vec3 view_vector = normalize(world_position.xyz - uni_camera_position);
	vec3 neg_view_vector = view_vector * (-1);
	vec3 normalized_normal = normalize((uni_M * vec4(normal, 0.0)).xyz);
	vec3 reflected_light = reflect(uni_light_direction, normalized_normal);
	
	float specular_factor = clamp(dot(reflected_light, neg_view_vector), 0, 1);
	specular_factor = pow(specular_factor, 5.0);
	pass_specular_colour = uni_light_colour * specular_factor;
    
	
}