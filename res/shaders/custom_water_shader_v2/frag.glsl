#version 330

in vec3 pass_colour;
in vec2 pass_uv;
in vec3 pass_normal;
in vec3 pass_world_position;

out vec4 final_colour;

uniform vec3 uni_camera_pos;
uniform vec3 fog_colour;
uniform float fog_density;

void main()
{
    vec3 blue_color = vec3(0.0, 0.3, 0.6); // Blue color for water
    float alpha = 0.5; // Semi-transparent

    float distance = length(pass_world_position - uni_camera_pos);
    float fog_factor = exp(-fog_density * distance * distance); // Exponential fog
    fog_factor = clamp(fog_factor, 0.0, 1.0);

    vec3 finalColor = mix(fog_colour, blue_color, fog_factor);

    final_colour = vec4(finalColor, alpha);
}