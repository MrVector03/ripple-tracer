#version 330

in vec3 pass_colour;
in vec2 pass_uv;
in vec3 pass_normal;

out vec4 final_colour;

uniform sampler2D albedo;

uniform float uni_phase;

void main()
{
	vec3 normalized_normal = normalize(pass_normal);
	// Tri puta citamo iz teksture sa blago razlicitih pozicija. Koordinate
	// citanja nisu tacno 'pass_uv' kako smo dobili, vec radimo jednostavne 'wave'
	// distorzije pomjerajuci te koordinate sinusima i kosinusima, u odnosu na
	// 'uni_phase' koje u svakom novom frejmu updateujemo.
    vec4 albedo_sample1 = texture(albedo, pass_uv +
                               vec2(sin((pass_uv.x + pass_uv.y) * 10.0 + uni_phase),
                                    cos((pass_uv.x + pass_uv.y) * 10.0 + uni_phase)) * 0.02);

    vec4 albedo_sample2 = texture(albedo, pass_uv +
                               vec2(sin((pass_uv.x + pass_uv.y) * 11.0 + uni_phase),
                                    cos((pass_uv.x + pass_uv.y) * 13.0 + uni_phase)) * 0.02);

    vec4 albedo_sample3 = texture(albedo, pass_uv +
                               vec2(sin((pass_uv.x + pass_uv.y) * 12.0 + uni_phase),
                                    cos((pass_uv.x + pass_uv.y) *  9.0 + uni_phase)) * 0.02);

    // Konacnu boju sastavljamo iz ova tri uzorka, tako sto iz njih redom uzimamo
    // samo R, G i B komponentu, sto ce izgledati kao da se ta tri sloja slike
    // nezavisno talasaju.
    final_colour.r = albedo_sample1.r;
    final_colour.g = albedo_sample2.g;
    final_colour.b = albedo_sample3.b;
	
}