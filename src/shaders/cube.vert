#version 430 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 norm;
layout (location = 2) in vec2 uv;

layout(std140, binding = 0) uniform world {
	mat4 projection;
	vec3 cam_position;
};

struct Instance {
	mat4 transform;
	vec4 color;
};

layout(std430, binding = 0) buffer md
{
	Instance instances[];
} models;

out vec3 frag_pos;
out vec3 normal;
out vec2 tex_uv;
out vec4 color;

void main()
{
	Instance instance = models.instances[gl_InstanceID];
	frag_pos = vec3(instance.transform * vec4(pos, 1.0f));
	normal = mat3(transpose(inverse(instance.transform))) * norm;
	tex_uv = uv;
	color = instance.color;
	gl_Position = projection * vec4(frag_pos, 1.0f);
}
