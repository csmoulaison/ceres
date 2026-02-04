#version 430 core
layout (location = 0) in vec3 pos;

layout(std140, binding = 0) uniform world {
	mat4 projection;
	vec3 cam_position;
};

layout(std140, binding = 1) uniform instance {
	mat4 model;
};

void main()
{
	//vec3 frag_pos = vec3(model * vec4(pos, 1.0f));
	//gl_Position = projection * vec4(frag_pos, 1.0f);
	gl_Position = projection * model * vec4(pos, 1.0f);
}
