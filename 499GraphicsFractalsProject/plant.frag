#version 430 core

struct Material 
{
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float shininess;
};

struct Light 
{
	vec3 position;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

layout (location = 0) in vec3 in_Position;
layout (location = 1) in vec3 in_Normal;
layout (location = 2) in vec3 in_Color;

layout (location = 0) out vec4 out_Color;

uniform vec3 viewPosition;
uniform Light light;
uniform Material material;

vec3 overlay(vec3 base, vec3 blend)
{
    vec3 multiply = 2.0 * base * blend;
    vec3 screen   = 1.0 - 2.0 * (1.0 - base) * (1.0 - blend);
    return mix(multiply, screen, step(0.5, base));
}

void main() 
{
	vec3 N = gl_FrontFacing ? normalize(in_Normal) : -normalize(in_Normal);
	
	vec3 L = normalize(light.position);
	vec3 E = normalize(viewPosition - in_Position);
	vec3 H = normalize(L + E);

	float Kd = max(dot(L, N), 0.0);
	float Ks = Kd > 0 ? pow(max(dot(N, H), 0.0), material.shininess) : 0.0;
	
	vec3 c = in_Color.rgb;

	vec3 ambient = material.ambient * light.ambient * c;
	vec3 specular = Ks * light.specular * material.specular;
	vec3 diffuse = light.diffuse * material.diffuse * c * Kd * 2;

	out_Color = vec4((ambient + diffuse + specular), 1.0);

}
