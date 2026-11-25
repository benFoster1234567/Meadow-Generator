#pragma once
#include "GLDebugHandler.h"

#include "Material.h"
#include <GL/glew.h>
#include <string_view>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <glm/gtc/type_ptr.hpp>
#include <glm/ext/matrix_float4x4.hpp>

#include "Light.h"

class SubroutineConfig
{
	GLint m_location{};
	GLuint m_index{};

public:
	SubroutineConfig(GLint location, GLuint index) : m_location(location), m_index(index) {}

	GLint getLocation() const { return m_location; }
	GLuint getIndex() const { return m_index; }
};

class Shader
{

	inline static void registerSubroutine(
		GLenum shaderType,
		GLuint& program,
		std::initializer_list<std::string_view> uniformNames,
		std::vector<GLuint>& indices,
		std::unordered_map<std::string, GLint>& locations)
	{
		glGetProgramStageiv(program, shaderType, GL_ACTIVE_SUBROUTINE_UNIFORMS, &activeUniformCount);
		if (activeUniformCount < 0 || static_cast<size_t>(activeUniformCount) != uniformNames.size())
		{
			throw std::logic_error("Mismatch between active subroutine uniform count and provided names.");
		}

		const size_t size = static_cast<size_t>(activeUniformCount);
		indices.assign(size, GL_INVALID_INDEX);
		//locations.clear();
		//locations.reserve(size);

		for (std::string_view name : uniformNames)
		{
			std::string nameStr(name);
			GLint loc = glGetSubroutineUniformLocation(program, shaderType, nameStr.c_str());

			if (loc < 0)
			{
				throw std::invalid_argument("Subroutine uniform not found or optimized out: " + nameStr);
			}

			indices.at(static_cast<size_t>(loc)) = GL_INVALID_INDEX;

			if (!locations.emplace(std::move(nameStr), loc).second)
			{
				throw std::invalid_argument("Duplicate subroutine uniform name registered.");
			}
		}
	}



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

	//subroutines-----------------------------------------------------

	//for vertex shader-----------------------------------------------
	void registerSubroutineVertexShader(std::initializer_list<std::string_view> uniformNames)
	{
		if (m_vertSubroutinesLoaded) throw std::logic_error("subroutine names already uploaded");

		registerSubroutine(
			GL_VERTEX_SHADER,
			m_program,
			uniformNames,
			m_subroutineIndicesVertex,
			m_locations);

		m_vertSubroutinesLoaded = true;
	}

	void setSubroutineVertex(std::string_view subroutineName, std::string_view uniformName)
	{
		GLint location = m_locations[std::string(subroutineName)];
		m_subroutineIndicesVertex[location] = glGetSubroutineIndex(m_program, GL_VERTEX_SHADER, uniformName.data());
	}

	void setSubroutineVertex(const SubroutineConfig& selection)
	{
		m_subroutineIndicesVertex[selection.getLocation()] = selection.getIndex();
	}

	SubroutineConfig getVertexSubroutineInfoForFastLookup(std::string_view subroutineName, std::string_view uniformName)
	{
		GLint location = m_locations[std::string(subroutineName)];
		auto index = glGetSubroutineIndex(m_program, GL_VERTEX_SHADER, uniformName.data());
		return { location, index };
	}



	//for fragment shader ----------------------------------------------------------

	void registerSubroutineFragmentShader(std::initializer_list<std::string_view> uniformNames)
	{
		if (m_fragSubroutinesLoaded) throw std::logic_error("subroutine names already uploaded");

		registerSubroutine(
			GL_FRAGMENT_SHADER,
			m_program,
			uniformNames,
			m_subroutineIndicesFragment,
			m_locations);

		m_fragSubroutinesLoaded = true;
	}

	void setSubroutineFragment(std::string_view subroutineName, std::string_view uniformName)
	{
		GLint location = m_locations[std::string(subroutineName)];
		m_subroutineIndicesFragment[location] = glGetSubroutineIndex(m_program, GL_FRAGMENT_SHADER, uniformName.data());
	}

	void setSubroutineFragment(const SubroutineConfig& selection)
	{
		m_subroutineIndicesFragment[selection.getLocation()] = selection.getIndex();
	}

	SubroutineConfig getFragmentSubroutineInfoForFastLookup(std::string_view subroutineName, std::string_view uniformName)
	{
		GLint location = m_locations[std::string(subroutineName)];
		auto index = glGetSubroutineIndex(m_program, GL_FRAGMENT_SHADER, uniformName.data());
		return { location, index };
	}



	void uploadSubroutines()
	{
		if (m_fragSubroutinesLoaded)
		{
			glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, m_subroutineIndicesFragment.size(), m_subroutineIndicesFragment.data());
		}

		if (m_vertSubroutinesLoaded)
		{
			glUniformSubroutinesuiv(GL_VERTEX_SHADER, m_subroutineIndicesVertex.size(), m_subroutineIndicesVertex.data());
		}
	}




	//uniforms-----------------------------------------------------

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

	void setUInt(const char* name, unsigned int value) const
	{
		auto loc = glGetUniformLocation(m_program, name);
		if (loc == -1)
			std::cerr << "location not found for " + std::string(name) + "\n";
		glUniform1ui(loc, value);
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

	//load from file --------------------------------------------------------------

	bool loadShader(std::string_view vertexFilePath, std::string_view fragmentFilePath);

	bool loadShader(std::pair<std::string_view, std::string_view> filenames)
	{
		return loadShader(filenames.first, filenames.second);
	}

private:
	GLuint m_program{ 0 };
	GLboolean m_isLoaded{ false };
	GLboolean m_fragSubroutinesLoaded{ false };
	GLboolean m_vertSubroutinesLoaded{ false };

	std::vector<GLuint> m_subroutineIndicesVertex{};
	std::vector<GLuint> m_subroutineIndicesFragment{};

	void destroy();

	mutable std::unordered_map<std::string, GLint> m_locations{};

	GLint location(const char* name) const
	{
		auto it = m_locations.find(name);
		if (it != m_locations.end()) return it->second;

		GLint loc = glGetUniformLocation(m_program, name);
		if (loc == -1)
		{
			DBG_LOG(DebugMsgType::GLWarning, "Warning: uniform " + std::string(name) + " not found(or optimized out)");
		}

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