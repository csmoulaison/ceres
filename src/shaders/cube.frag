#version 430 core

in vec3 frag_pos;
in vec3 normal;
in vec2 tex_uv;
in vec4 color;
in vec3 view_pos;

out vec4 frag_color;

uniform sampler2D tex;

void main()
{
	vec3 light_pos = vec3(200.0f, 100.5f, 400.0f);
	vec3 light_color = vec3(1.0f, 1.0f, 1.0f);
	vec3 ambient = vec3(0.2f, 0.2f, 0.2f);

	vec3 norm = normalize(normal);
	vec3 light_dir = normalize(light_pos - frag_pos);
	float diff = max(dot(norm, light_dir), 0.0f);
	vec3 diffuse = diff * light_color;

	// specular
	float spec_strength = 10.0f;
	vec3 view_dir = normalize(light_pos - frag_pos);
	vec3 reflect_dir = reflect(-light_dir, norm);
	float spec = pow(max(dot(view_dir, reflect_dir), 0.0f), 32);
	vec3 specular = spec_strength * spec * light_color;

	vec3 result = ambient + diffuse + specular;
	vec4 f_color = vec4(result, 1.0f) * texture(tex, tex_uv);
	vec4 color_4 = vec4(color.xyz, 1.0f);
    frag_color = mix(f_color, color_4, color.w);
    // vec4(normal * vec3(0.8f, 0.0f, 0.8f), 0.0f);
    //frag_color = vec4(result, 1.0f);
	//frag_color = vec4(1.0f);
    //frag_color = texture(tex, tex_uv);
}
