#version 430 core

layout (location = 0) in vec3 in_Position;
layout (location = 0) out uint out_Color;

uniform uint index;

void main()
{
	out_Color = index;
}