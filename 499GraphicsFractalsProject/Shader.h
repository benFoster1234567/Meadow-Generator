#pragma once
#include <GL/glew.h>
#include <string_view>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/ext/matrix_float4x4.hpp>
class Shader
{
public:

	Shader() = default;

	Shader(const Shader&) = delete;
	Shader& operator=(const Shader&) = delete;
	
	Shader(Shader&& other) noexcept
		: m_isLoaded(std::exchange(other.m_isLoaded, 0))
		, m_program(std::exchange(other.m_program, 0))
	{
	}

	Shader& operator=(Shader&& other) noexcept
	{
		m_program = std::exchange(other.m_program, 0);
		return *this;
	}

	~Shader()
	{
		destroy();
	}

	void useShader() const
	{
		glUseProgram(m_program);
	}

	void setInt(const char* name, GLint value) const
	{
		glUniform1i(glGetUniformLocation(m_program, name), value);
	}

	void setFloat(const char* name, GLfloat value) const
	{
		glUniform1f(glGetUniformLocation(m_program, name), value);
	}

	void setVec3(const char* name, glm::vec3 value) const
	{
		glUniform3fv(glGetUniformLocation(m_program, name), 1, (GLfloat*)&value[0]);
	}

	void setMat4(const char* name, glm::mat4 value) const
	{
		glUniformMatrix4fv(glGetUniformLocation(m_program, name), 1, GL_FALSE, (GLfloat*)&value[0]);
	}

	void setMat3(const char* name, glm::mat3 value) const
	{
		glUniformMatrix3fv(glGetUniformLocation(m_program, name), 1, GL_FALSE, (GLfloat*)&value[0]);
	}

	bool loadShader(std::string_view vertexFilePath, std::string_view fragmentFilePath)
	{
		m_program = compileShader(readFile(vertexFilePath), readFile(fragmentFilePath));
	}

	bool loadShader(std::string_view shader)
	{
		return false;
	}

private:
	GLuint m_program{ 0 };
	GLboolean m_isLoaded{ false };
	
	void destroy()
	{
		glUseProgram(0);
		if (m_program != 0)
		{
			glDeleteProgram(this->m_program);
			this->m_program = 0;
		}
	}

	static const char* readFile(std::string_view filePath)
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

		return ss.str().c_str();
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