#pragma once
#include <string>
#include <sstream>
#include <GL/glew.h>
#include <iostream>

#include <IL/il.h>
#include <IL/ilu.h>
#include <IL/ilut.h>

#define GLM_FORCE_RADIANS

#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Material.h"

void setInt(const char* name, GLint value, GLuint& program) {
	glUniform1i(glGetUniformLocation(program, name), value);
}

void setFloat(const char* name, GLfloat value, GLuint& program) {
	glUniform1f(glGetUniformLocation(program, name), value);
}

void setVec3(const char* name, glm::vec3 value, GLuint& program) {
	glUniform3fv(glGetUniformLocation(program, name), 1, (GLfloat*)&value[0]);
}

void setMat4(const char* name, glm::mat4 value, GLuint& program) {
	glUniformMatrix4fv(glGetUniformLocation(program, name), 1, GL_FALSE, (GLfloat*)&value[0]);
}

void setMat3(const char* name, glm::mat3 value, GLuint& program) {
	glUniformMatrix3fv(glGetUniformLocation(program, name), 1, GL_FALSE, (GLfloat*)&value[0]);
}

void setMaterial(Material mat, GLuint program) {
	setVec3("material.ambient", mat.ambient, program);
	setVec3("material.diffuse", mat.diffuse, program);
	setVec3("material.specular", mat.specular, program);
	setFloat("material.shininess", mat.shininess, program);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mat.diffuseMap);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, mat.specularMap);
}


GLuint initializeShaders(const char* v_shaderString, const char* f_shaderString)
{
	GLint success;
	char infoLog[512];

	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	GLuint shaderProgram = glCreateProgram();

	glShaderSource(vertexShader, 1, &v_shaderString, NULL);
	glCompileShader(vertexShader);

	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);

	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cerr << "ERROR: Vertex Shader Compilation Failed\n" << infoLog << std::endl;
	}

	glShaderSource(fragmentShader, 1, &f_shaderString, NULL);
	glCompileShader(fragmentShader);

	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);

	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cerr << "ERROR: Fragment Shader Compilation Failed\n" << infoLog << std::endl;
	}

	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);

	glLinkProgram(shaderProgram);

	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cerr << "ERROR: Shader Program Linking Failed\n" << infoLog << std::endl;
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return shaderProgram;
}




