#version 430 core

in vec3 frag_pos;
in vec3 normal;

out vec4 frag_color;

void main()
{
	vec3 light_pos = vec3(2.0f, 0.5f, 2.0f);
	vec3 light_color = vec3(1.0f, 1.0f, 1.0f);
	vec3 object_color = vec3(0.6f, 0.6f, 0.6f);
	vec3 ambient = vec3(0.1f);

	vec3 norm = normalize(normal);
	vec3 light_direction = normalize(light_pos - frag_pos);
	float diff = max(dot(norm, light_direction), 0.0f);
	vec3 diffuse = diff * light_color;

	vec3 result = (ambient + diffuse) * object_color;
	frag_color = vec4(result, 1.0f);
}
