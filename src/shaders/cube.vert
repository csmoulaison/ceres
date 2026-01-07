#version 430 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 norm;

layout(std140, binding = 0) uniform ubo {
	mat4 projection;
	mat4 model;
	vec4 cam_position;
};

out vec3 frag_pos;
out vec3 normal;

void main()
{
	frag_pos = vec3(model * vec4(pos, 1.0f));
	normal = mat3(transpose(inverse(model))) * norm;
	//normal = norm;
	gl_Position = projection * vec4(frag_pos, 1.0f);
}
