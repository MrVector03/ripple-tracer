#version 330

in vec2 frag_uv;
in vec3 frag_position;

out vec4 final_colour;

uniform sampler2D reflectionTexture;
uniform sampler2D refractionTexture;
uniform vec3 camera_pos;
uniform float time;
uniform float refraction_index;

vec3 calculateNormal(vec3 frag_position, float time) {
    vec3 normal = vec3(
    sin(frag_position.x * 5.0 + time) * 0.1,
    1.0,
    sin(frag_position.z * 5.0 + time) * 0.1
    );
    return normalize(normal);
}

void main()
{
    vec3 normal = calculateNormal(frag_position, time);

    vec3 view_dir = normalize(camera_pos - frag_position);

    vec3 reflect_dir = reflect(-view_dir, normal);
    vec3 refract_dir = refract(-view_dir, normal, 1.0 / refraction_index);

    vec2 reflect_tex_coords = (reflect_dir.xy) * 0.5 + 0.5;
    vec2 refract_tex_coords = (refract_dir.xy) * 0.5 + 0.5;

    reflect_tex_coords = clamp(reflect_tex_coords, 0.0, 1.0);
    refract_tex_coords = clamp(refract_tex_coords, 0.0, 1.0);

    vec4 reflect_color = texture(reflectionTexture, reflect_tex_coords);
    vec4 refract_color = texture(refractionTexture, refract_tex_coords);

    float fresnel = pow(1.0 - dot(view_dir, normal), 5.0);

    vec4 final_reflection = mix(refract_color, reflect_color, fresnel);

    final_colour = final_reflection;
}
