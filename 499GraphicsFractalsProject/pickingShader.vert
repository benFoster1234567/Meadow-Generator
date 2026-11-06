#version 430 core

layout (location = 0) in vec3 in_Position;
layout (location = 1) in vec3 in_Normal;
layout (location = 2) in vec2 in_TexCoord0;

uniform mat4 model;
uniform mat4 projection;
uniform mat4 view;

void main()
{
	vec3 worldPosition = vec3(model * vec4(in_Position, 1.0));
	gl_Position = projection * view * vec4(worldPosition, 1.0);
}