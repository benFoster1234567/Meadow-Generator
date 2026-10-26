#pragma once
#include <vector>
#include <memory>
#include "Shader.h"
#include <optional>
#include "Camera.h"

class ShaderManager
{
	std::vector<std::shared_ptr<Shader>> shaders;


public:
	using ShaderIndex = size_t;

	void useShader(const ShaderIndex& shaderIdx)
	{
		shaders[shaderIdx]->useShader();
	}

	/// <summary>
	/// allows for 
	/// </summary>
	class ShaderGroup
	{
	private:
		std::vector<std::shared_ptr<Shader>>& m_shaders;
		ShaderGroup(std::vector <std::shared_ptr<Shader>>& shaders)
			: m_shaders{ shaders }
		{
		}

		friend class ShaderManager;
	public:

		ShaderGroup(const ShaderGroup&) = delete;
		ShaderGroup(ShaderGroup&&) noexcept = default;
		ShaderGroup& operator=(const ShaderGroup&) = delete;
		ShaderGroup& operator=(ShaderGroup&&) = delete;
		~ShaderGroup() = default;

		void setInt(const char* name, GLint value)
		{
			for (const auto& shader : m_shaders)
			{
				shader->useShader();
				shader->setInt(name, value);
			}
		}

		void setFloat(const char* name, GLfloat value) const
		{
			for (const auto& shader : m_shaders)
			{
				shader->useShader();
				shader->setFloat(name, value);
			}
		}

		void setVec3(const char* name, glm::vec3 value) const
		{
			for (const auto& shader : m_shaders)
			{
				shader->useShader();
				shader->setVec3(name, value);
			}
		}

		void setMat4(const char* name, glm::mat4 value) const
		{
			for (const auto& shader : m_shaders)
			{
				shader->useShader();
				shader->setMat4(name, value);
			}
		}

		void setMat3(const char* name, glm::mat3 value) const
		{
			for (const auto& shader : m_shaders)
			{
				shader->useShader();
				shader->setMat3(name, value);
			}
		}

		void setLight(const Light& light)
		{
			for (const auto& shader : m_shaders)
			{
				shader->useShader();
				shader->setLight(light);
			}
		}


	};

	ShaderGroup all()
	{
		return ShaderGroup(shaders);
	}

	ShaderIndex addShader(std::string_view vertexFilePath, std::string_view fragmentFilePath)
	{
		auto shader = std::make_shared<Shader>();
		shader->loadShader(vertexFilePath, fragmentFilePath);
		shaders.push_back(std::move(shader));
		return shaders.size() - 1;
	}

	std::optional<std::weak_ptr<Shader>> getShader(ShaderIndex i)
	{
		if (i >= shaders.size())
		{
			return std::nullopt;
		}

		return shaders[i];
	}

	void uploadCameraUniforms(const Camera& camera, std::string_view viewName
		, std::string_view projName
		, std::string_view eyeName)
	{
		for (const auto& shader : shaders)
		{
			shader->useShader();
			camera.setUniformProj(projName, shader);
			camera.setUniformView(viewName, shader);
			camera.setUniformPos(eyeName, shader);
		}
	}


};