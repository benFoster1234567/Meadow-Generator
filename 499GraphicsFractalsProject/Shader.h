#pragma once

#include "Material.h"
#include <GL/glew.h>
#include <string_view>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include "Light.h"
class Shader
{
public:
	Shader() = default;
	Shader(const Shader&) = delete;
	Shader& operator=(const Shader&) = default;
	Shader(Shader&& other) noexcept;
	Shader& operator=(Shader&& other) noexcept;
	~Shader();

	void useShader() const
	{
		glUseProgram(m_program);
	}

	void setInt(const char* name, GLint value) const
	{
		glUniform1i(location(name), value);
	}

	void setFloat(const char* name, GLfloat value) const
	{
		glUniform1f(location(name), value);
	}

	void setVec3(const char* name, const glm::vec3& value) const
	{
		glUniform3fv(location(name), 1, glm::value_ptr(value));
	}

	void setMat3(const char* name, const glm::mat3& value) const
	{
		glUniformMatrix3fv(location(name), 1, GL_FALSE, glm::value_ptr(value));
	}

	void setMat4(const char* name, const glm::mat4& value) const
	{
		glUniformMatrix4fv(location(name), 1, GL_FALSE, glm::value_ptr(value));
	}

	void setLight(Light light) const
	{
		setVec3("light.position", light.position);
		setVec3("light.ambient", light.ambient);
		setVec3("light.diffuse", light.diffuse);
		setVec3("light.specular", light.specular);
	}

	void bindTexture(const char* name, size_t slot, GLuint texture) const
	{
		glActiveTexture(GL_TEXTURE0 + slot);
		glBindTexture(GL_TEXTURE_2D, texture);

		useShader();

		setInt(name, slot);
	}

	void setMaterial(Material material, bool sendMaps = true) const
	{
		setVec3("material.ambient", material.ambient);
		setVec3("material.diffuse", material.diffuse);
		setVec3("material.specular", material.specular);
		setFloat("material.shininess", material.shininess);
		if (!sendMaps) return;
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, material.diffuseMap);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, material.specularMap);
	}

	bool loadShader(std::string_view vertexFilePath, std::string_view fragmentFilePath);

private:
	GLuint m_program{ 0 };
	GLboolean m_isLoaded{ false };
	void destroy();

	mutable std::unordered_map<std::string, GLint> m_locations;

	GLint location(const char* name) const
	{
		auto it = m_locations.find(name);
		if (it != m_locations.end()) return it->second;

		GLint loc = glGetUniformLocation(m_program, name);
		if (loc == -1)
			fprintf(stderr, "Warning: uniform '%s' not found (or optimized out)\n", name);
		m_locations.emplace(name, loc);
		return loc;
	}


	static std::string readFile(std::string_view filePath)
	{
		std::ifstream fileIn(filePath.data());
		std::stringstream ss{};

		if (!fileIn.is_open())
		{
			throw std::runtime_error("shader file failed to open");
		}

		std::string line;

		while (std::getline(fileIn, line))
		{
			ss << line << "\n";
		}

		return ss.str();
	}

	static GLuint compileShader(const char* v_shaderString, const char* f_shaderString)
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

};