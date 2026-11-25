#include "Shader.h"

Shader::Shader(Shader&& o) noexcept
	: m_program(std::exchange(o.m_program, 0))
	, m_isLoaded(std::exchange(o.m_isLoaded, false))
	, m_fragSubroutinesLoaded(std::exchange(o.m_fragSubroutinesLoaded, false))
	, m_vertSubroutinesLoaded(std::exchange(o.m_vertSubroutinesLoaded, false))
	, m_subroutineIndicesVertex(std::move(o.m_subroutineIndicesVertex))
	, m_subroutineIndicesFragment(std::move(o.m_subroutineIndicesFragment))
	, m_locations(std::move(o.m_locations))
{
}

Shader& Shader::operator=(Shader&& o) noexcept
{
	if (this != &o)
	{
		destroy();
		m_program = std::exchange(o.m_program, 0);
		m_isLoaded = std::exchange(o.m_isLoaded, false);
		m_fragSubroutinesLoaded = o.m_fragSubroutinesLoaded;
		m_vertSubroutinesLoaded = o.m_vertSubroutinesLoaded;
		m_subroutineIndicesVertex = std::move(o.m_subroutineIndicesVertex);
		m_subroutineIndicesFragment = std::move(o.m_subroutineIndicesFragment);
		m_locations = std::move(o.m_locations);
	}
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
		m_program = 0;
	}
}
