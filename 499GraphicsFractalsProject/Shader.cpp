#include "Shader.h"

Shader::Shader(Shader&& other) noexcept
	: m_isLoaded(std::exchange(other.m_isLoaded, 0))
	, m_program(std::exchange(other.m_program, 0))
{
}

Shader& Shader::operator=(Shader&& other) noexcept
{
	m_program = std::exchange(other.m_program, 0);
	return *this;
}

Shader::~Shader()
{
	destroy();
}

bool Shader::loadShader(std::string_view vertexFilePath, std::string_view fragmentFilePath)
{
	std::string vs = readFile(vertexFilePath);
	std::string fs = readFile(fragmentFilePath);
	m_program = compileShader(vs.c_str(), fs.c_str());
	if (m_program == 0) throw std::runtime_error("shader not loaded!");
	return true;
}

void Shader::destroy()
{
	glUseProgram(0);
	if (m_program != 0)
	{
		glDeleteProgram(this->m_program);
		this->m_program = 0;
	}
}
