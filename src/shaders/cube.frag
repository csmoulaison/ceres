#version 430 core

in vec3 frag_pos;
in vec3 normal;
in vec2 tex_uv;

out vec4 frag_color;

uniform sampler2D tex;

void main()
{
	vec3 light_pos = vec3(200.0f, 100.5f, 400.0f);
	vec3 light_color = vec3(1.0f, 1.0f, 1.0f);
	vec3 ambient = vec3(0.2f, 0.2f, 0.2f);

	vec3 norm = normalize(normal);
	vec3 light_direction = normalize(light_pos - frag_pos);
	float diff = max(dot(norm, light_direction), 0.0f);
	vec3 diffuse = diff * light_color;

	vec3 result = ambient + diffuse;
    frag_color = vec4(result, 1.0f) * texture(tex, tex_uv);
    // vec4(normal * vec3(0.8f, 0.0f, 0.8f), 0.0f);
    //frag_color = vec4(result, 1.0f);
	//frag_color = vec4(1.0f);
    //frag_color = texture(tex, tex_uv);
}
