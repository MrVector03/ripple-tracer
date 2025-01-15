#version 330 core

out vec4 final_colour;

in vec3 tex_coords;

uniform samplerCube skybox;
uniform vec3 fog_color;
uniform float fog_density;
uniform vec3 view_position;

void main()
{
    vec4 skybox_color = texture(skybox, tex_coords);

    float distance = length(view_position);
    if (fog_density == 0.0)
    {
        final_colour = skybox_color;
        return;
    }
    float fog_factor = exp(-fog_density * distance);
    fog_factor = clamp(fog_factor, 0.0, 1.0);

    vec3 final_color = mix(fog_color, skybox_color.rgb, fog_factor);

    final_colour = vec4(final_color, skybox_color.a);
}