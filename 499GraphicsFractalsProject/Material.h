#pragma once
#include <gl/glew.h>
#include <glm/ext/vector_float3.hpp>

typedef struct
{
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
	GLfloat shininess;
	GLuint specularMap;
	GLuint diffuseMap;
}
Material;
