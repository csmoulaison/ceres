#version 430 core
in vec4 color;

out vec4 frag_color;

uniform sampler2D tex;

void main()
{
	frag_color = color;
}
