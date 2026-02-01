#version 430 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 norm;
layout (location = 2) in vec2 uv;

layout(std140, binding = 0) uniform world {
	mat4 projection;
	vec3 cam_position;
};

layout(std140, binding = 1) uniform instance {
	mat4 model;
};

out vec3 frag_pos;
out vec3 normal;
out vec2 tex_uv;

void main()
{
	frag_pos = vec3(model * vec4(pos, 1.0f));
	normal = mat3(transpose(inverse(model))) * norm;
	tex_uv = uv;
	gl_Position = projection * vec4(frag_pos, 1.0f);
}
