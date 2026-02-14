#version 430 core
layout (location = 0) in vec2 pos;

struct Rect {
	vec4 dst;
	vec4 color;
};

layout(std430, binding = 0) buffer rt
{
	Rect instances[];
} rects;

out vec3 frag_pos;
out vec4 color;

void main()
{
	vec2 norm_pos = pos / vec2(2.0f) + vec2(0.5f);
	Rect rect = rects.instances[gl_InstanceID];

	vec2 pos2d = rect.dst.xy + rect.dst.zw * norm_pos;
	gl_Position = vec4(pos2d, 0.0f, 1.0f);

	color = rect.color;
}
