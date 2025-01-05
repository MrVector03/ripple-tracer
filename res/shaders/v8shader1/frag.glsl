#version 330

in vec3 pass_colour;
in vec2 pass_uv;
in vec3 pass_normal;

out vec4 final_colour;

// sampler2D je "handle" preko kog mozemo uzeti uzorak iz 2D teksture
// Nacin na koji se uzorak uzima je definisam parametrima date teksture
// (filtriranje, ponavljanje, mipmape, itd.)
// slicno kao i rafgl_bilinear_sample ili rafgl_point_sample
uniform sampler2D albedo;

void main()
{
	vec3 normalized_normal = normalize(pass_normal);
	
	// Uzimamo uzorak iz bajndovane teksture na koordinatama iz pass_uv i to
	// direktno upisujemo kao konacnu boju piksela
	vec4 albedo_colour = texture(albedo, pass_uv * 0.5 + vec2(0.25, 0.25));
	final_colour = albedo_colour;
	
}