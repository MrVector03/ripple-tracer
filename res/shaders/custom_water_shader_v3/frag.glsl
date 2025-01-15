#version 330 core

in vec2 pass_uv;
in vec3 pass_world_position;

out vec4 final_colour;

uniform sampler2D reflection_texture;
uniform vec3 uni_camera_pos;

void main()
{
    // Calculate view direction
    vec3 view_dir = normalize(pass_world_position - uni_camera_pos);

    // Reflection vector
    vec3 reflect_dir = reflect(view_dir, vec3(0.0, 1.0, 0.0));

    // Properly scale UV coordinates for reflection
    vec2 reflect_uv = pass_uv + reflect_dir.xy * 1; // Adjust scaling factor as needed

    // Clamp UV coordinates to avoid sampling outside texture bounds
    reflect_uv = clamp(reflect_uv, 0.0, 1.0);

    // Sample the reflection texture
    vec4 reflection_color = texture(reflection_texture, reflect_uv);

    // Set the final color to the reflection color
    final_colour = vec4(reflection_color.rgb, 0.5);  // Adjust alpha as needed
}