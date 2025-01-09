#version 330

in vec2 frag_uv;
in vec3 frag_position;

out vec4 final_colour;

uniform sampler2D reflectionTexture;
uniform vec3 camera_pos;
uniform float time;

void main()
{
    // Calculate normal for simple wave effect
    vec3 normal = vec3(
    sin(frag_position.x * 0.1 + time) * 0.1,
    1.0,
    sin(frag_position.z * 0.1 + time) * 0.1
    );
    normal = normalize(normal);

    // Calculate reflection vector
    vec3 view_dir = normalize(frag_position - camera_pos);
    vec3 reflect_dir = reflect(view_dir, normal);

    // Convert reflection vector to texture coordinates
    vec2 reflect_tex_coords = (reflect_dir.xy * 0.5) + 0.5;

    // Sample the reflection texture
    vec4 reflect_color = texture(reflectionTexture, reflect_tex_coords);

    // Output the reflection color
    final_colour = reflect_color;
}