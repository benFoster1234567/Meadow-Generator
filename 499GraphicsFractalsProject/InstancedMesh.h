#pragma once
#include "GL/glew.h"
#include "Mesh.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

struct InstanceData
{
	glm::vec3 position{};
	glm::vec3 rotationEuler{};
	glm::vec3 scale{};
	glm::vec4 color{};
};

class InstancedMesh
{
private:
	void release()
	{
		if (m_vbo) glDeleteBuffers(1, &m_vbo);
		if (m_ebo) glDeleteBuffers(1, &m_ebo);
		if (m_ibo) glDeleteBuffers(1, &m_ibo);
		if (m_vao) glDeleteVertexArrays(1, &m_vao);
		m_vao = m_vbo = m_ebo = m_ibo = 0;
	}
	std::vector<glm::mat4> instanceData{};

	glm::mat4 packInstance(const InstanceData& instance)
	{
		glm::mat4 pos = glm::translate(glm::mat4(1), instance.position);
		glm::mat4 rot = glm::toMat4(glm::quat(instance.rotationEuler));
		glm::mat4 scale = glm::scale(glm::mat4(1), instance.scale);

		glm::mat4 transform = pos * rot * scale;

		transform[0].w = instance.color.r;
		transform[1].w = instance.color.g;
		transform[2].w = instance.color.b;
		transform[3].w = instance.color.a;

		return transform;
	}

	GLuint m_vao{};
	GLuint m_vbo{};
	GLuint m_ebo{};
	GLuint m_ibo{};

	bool m_dirty = false;
	bool m_initialized = false;

public:

	Mesh mesh{};
	GLuint shaderProgram{};
	InstancedMesh() = default;

	explicit InstancedMesh(const Mesh& mesh, GLuint shaderProgram = 0)
		: shaderProgram(shaderProgram), mesh(mesh) {
	}

	explicit InstancedMesh(Mesh&& mesh, GLuint shaderProgram = 0)
		: shaderProgram(shaderProgram), mesh(std::move(mesh)) {
	}

	InstancedMesh(const Mesh& mesh, std::vector<glm::mat4> instances, GLuint shaderProgram = 0)
		: shaderProgram(shaderProgram), mesh(mesh), instanceData(std::move(instances)) {
	}

	InstancedMesh(const Mesh& mesh, const std::vector<InstanceData>& instances, GLuint shaderProgram = 0)
		: shaderProgram(shaderProgram), mesh(mesh)
	{
		instanceData.reserve(instances.size());
		for (const auto& inst : instances)
			instanceData.push_back(packInstance(inst));
	}

	~InstancedMesh() { release(); }

	InstancedMesh(const InstancedMesh&) = delete;
	InstancedMesh& operator=(const InstancedMesh&) = delete;

	InstancedMesh(InstancedMesh&& other) noexcept
		: shaderProgram(std::exchange(other.shaderProgram, 0)),
		m_vao(std::exchange(other.m_vao, 0)),
		m_vbo(std::exchange(other.m_vbo, 0)),
		m_ebo(std::exchange(other.m_ebo, 0)),
		m_ibo(std::exchange(other.m_ibo, 0)),
		mesh(std::move(other.mesh)),
		instanceData(std::move(other.instanceData)) {
	}

	InstancedMesh& operator=(InstancedMesh&& other) noexcept
	{
		if (this != &other)
		{
			release();
			shaderProgram = std::exchange(other.shaderProgram, 0);
			m_vao = std::exchange(other.m_vao, 0);
			m_vbo = std::exchange(other.m_vbo, 0);
			m_ebo = std::exchange(other.m_ebo, 0);
			m_ibo = std::exchange(other.m_ibo, 0);
			mesh = std::move(other.mesh);
			instanceData = std::move(other.instanceData);
		}
		return *this;
	}

	void addInstance(const InstanceData& data)
	{
		instanceData.push_back(packInstance(data));
	}

	void addInstance(const glm::mat4& data)
	{
		instanceData.push_back(data);
	}

	void initialize()
	{
		glGenVertexArrays(1, &m_vao);
		glBindVertexArray(m_vao);
		glGenBuffers(1, &m_vbo);
		glGenBuffers(1, &m_ebo);
		glGenBuffers(1, &m_ibo);

		glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
		glBufferData(GL_ARRAY_BUFFER, mesh.m_data.size() * sizeof(VertexData), mesh.m_data.data(), GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, point));
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, normal));
		glEnableVertexAttribArray(1);

		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, texCoord));
		glEnableVertexAttribArray(2);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.m_indices.size() * sizeof(unsigned int), mesh.m_indices.data(), GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, m_ibo);
		glBufferData(GL_ARRAY_BUFFER, instanceData.size() * sizeof(glm::mat4), instanceData.data(), GL_DYNAMIC_DRAW);

		for (int i = 0; i < 4; ++i) // puts the whole matrix into the shaders as 4 vec4s
		{
			GLuint attribIndex = 3 + i;
			GLuint stride = sizeof(glm::mat4);
			GLuint size = 4;
			glVertexAttribPointer(attribIndex, size, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(glm::vec4) * i));
			glEnableVertexAttribArray(attribIndex);
			glVertexAttribDivisor(attribIndex, 1);
		}

		m_initialized = true;
		m_dirty = false;
		glBindVertexArray(0);
	}

	void draw()
	{
		glBindVertexArray(m_vao);
		glDrawElementsInstanced(GL_TRIANGLES, mesh.m_indices.size(), GL_UNSIGNED_INT, (void*)0, instanceData.size());
	}


};