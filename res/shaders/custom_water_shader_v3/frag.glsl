#version 330 core

in vec2 pass_uv;
in vec3 pass_world_position;

out vec4 final_colour;

uniform sampler2D reflection_texture;
uniform sampler2D refraction_texture;
uniform sampler2D normal_map;

uniform vec3 uni_camera_pos;
uniform float time_tick;

const float wave_speed = 0.03;

void main()
{
    // Calculate view direction
    vec3 view_dir = normalize(pass_world_position - uni_camera_pos);

    // Fetch normal from normal map
    vec3 normal = texture(normal_map, pass_uv + vec2(time_tick * wave_speed)).rgb;
    normal = normalize(normal * 2.0 - 1.0);  // Convert to [-1, 1] range

    // Reflection vector
    vec3 reflect_dir = reflect(view_dir, normal);

    // Properly scale UV coordinates for reflection
    vec2 reflect_uv = pass_uv + reflect_dir.xy * 0.1;

    // Clamp UV coordinates to avoid sampling outside texture bounds
    reflect_uv = clamp(reflect_uv, 0.0, 1.0);

    // Sample the reflection texture
    vec4 reflection_color = texture(reflection_texture, reflect_uv);

    // For simplicity, use the original UV for refraction
    vec4 refraction_color = texture(refraction_texture, pass_uv);

    // Fresnel factor
    float fresnel_factor = pow(1.0 - dot(view_dir, normal), 3.0);

    // Combine reflection and refraction based on the Fresnel factor
    vec4 water_color = mix(refraction_color, reflection_color, fresnel_factor);

    // Output the final color with adjusted transparency
    final_colour = vec4(water_color.rgb, 1.0);  // Set alpha to 1.0 for full opacity
}