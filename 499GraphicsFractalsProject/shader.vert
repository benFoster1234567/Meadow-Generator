#version 430 core

layout (location = 0) in vec3 in_Position;
layout (location = 1) in vec3 in_Normal;
layout (location = 2) in vec2 in_TexCoord0;
layout (location = 3) in mat4 in_InstanceData;

layout (location = 0) out vec3 out_Position;
layout (location = 1) out vec3 out_Normal;
layout (location = 2) out vec2 out_TexCoord0;
layout (location = 3) out vec3 out_color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 texture;

void main() 
{
	mat4 m = in_InstanceData;

	out_color = vec3(m[0].w, m[1].w, m[2].w);
	
	m[0].w = 0.0;
	m[1].w = 0.0;
	m[2].w = 0.0;
	m[3].w = 1.0;

	mat4 worldModel = m * model;

	vec3 worldPosition = vec3(worldModel * vec4(in_Position, 1.0));

	out_Position = worldPosition;

	//out_Normal = in_Normal;
	out_Normal = mat3(transpose(inverse(worldModel))) * in_Normal;
	//out_TexCoord0 = vec2(texture * vec3(in_TexCoord0, 1.0));
	out_TexCoord0 = in_TexCoord0;
	gl_Position = projection * view * vec4(worldPosition, 1.0);

}