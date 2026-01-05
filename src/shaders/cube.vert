#version 430 core
layout (location = 0) in vec3 pos;

layout(std140, binding = 0) uniform ubo {
	mat4 projection;
	mat4 model;
};

out vec4 color;

void main()
{
	gl_Position = projection * model * vec4(pos, 1.0f);
	color = vec4(1.0f);
}
