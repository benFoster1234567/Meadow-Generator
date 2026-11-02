#pragma once
#pragma once
#include "GL/glew.h"
#include "Mesh.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <vector>
#include <ranges>

class GpuMesh
{
protected:

	virtual void initializeAdditionalAttributes() {}
	virtual void releaseAdditionalAttributes() {}

	void release()
	{
		if (m_vbo) glDeleteBuffers(1, &m_vbo);
		if (m_ebo) glDeleteBuffers(1, &m_ebo);
		if (m_vao) glDeleteVertexArrays(1, &m_vao);
		releaseAdditionalAttributes();
		m_vbo = m_ebo = m_vao = 0;
	}

	GLuint m_vao{};
	GLuint m_vbo{};
	GLuint m_ebo{};

	bool m_dirty = false;
	bool m_initialized = false;

public:

	Mesh m_mesh{};
	GpuMesh() = default;

	explicit GpuMesh(const Mesh& mesh)
		: m_mesh(mesh) {
	}

	explicit GpuMesh(Mesh&& mesh)
		: m_mesh(std::move(mesh)) {
	}

	~GpuMesh() { release(); }

	GpuMesh(const GpuMesh&) = delete;
	GpuMesh& operator=(const GpuMesh&) = delete;

	GpuMesh(GpuMesh&& other) noexcept
		:
		m_vao(std::exchange(other.m_vao, 0)),
		m_vbo(std::exchange(other.m_vbo, 0)),
		m_ebo(std::exchange(other.m_ebo, 0)),
		m_initialized(other.m_initialized),
		m_dirty(other.m_dirty),
		m_mesh(std::move(other.m_mesh))
	{
	}

	GpuMesh& operator=(GpuMesh&& other) noexcept
	{
		if (this != &other)
		{
			release();
			m_vao = std::exchange(other.m_vao, 0);
			m_vbo = std::exchange(other.m_vbo, 0);
			m_ebo = std::exchange(other.m_ebo, 0);
			m_mesh = std::move(other.m_mesh);
		}
		return *this;
	}

	void initialize()
	{
		if (m_initialized) return;
		glGenVertexArrays(1, &m_vao);
		glBindVertexArray(m_vao);
		glGenBuffers(1, &m_vbo);
		glGenBuffers(1, &m_ebo);

		glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
		glBufferData(GL_ARRAY_BUFFER, m_mesh.m_data.size() * sizeof(VertexData), m_mesh.m_data.data(), GL_DYNAMIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, point));
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, normal));
		glEnableVertexAttribArray(1);

		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, texCoord));
		glEnableVertexAttribArray(2);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_mesh.m_indices.size() * sizeof(unsigned int), m_mesh.m_indices.data(), GL_DYNAMIC_DRAW);

		initializeAdditionalAttributes();

		m_initialized = true;
		m_dirty = false;
		glBindVertexArray(0);
	}

	void changeMesh(Mesh mesh)
	{
		bool sizeEquivalent = mesh.m_data.size() == m_mesh.m_data.size() && mesh.m_indices.size() == m_mesh.m_indices.size();
		m_mesh = std::move(mesh);

		if (!m_initialized) return;
		glBindVertexArray(m_vao);

		const GLsizeiptr vboBytes = m_mesh.m_data.size() * sizeof(VertexData);
		const GLsizeiptr eboBytes = m_mesh.m_indices.size() * sizeof(unsigned int);

		glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
		if (sizeEquivalent)
		{
			glBufferSubData(GL_ARRAY_BUFFER, 0, vboBytes, m_mesh.m_data.data());
		}
		else
		{
			glBufferData(GL_ARRAY_BUFFER, vboBytes, m_mesh.m_data.data(), GL_DYNAMIC_DRAW);
		}

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
		if (sizeEquivalent)
		{
			glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, eboBytes, m_mesh.m_indices.data());
		}
		else
		{
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, eboBytes, m_mesh.m_indices.data(), GL_DYNAMIC_DRAW);
		}

		glBindVertexArray(0);
	}

	virtual void draw()
	{
		glBindVertexArray(m_vao);
		glDrawElements(GL_TRIANGLES, m_mesh.m_indices.size(), GL_UNSIGNED_INT, (void*)0);
		glBindVertexArray(0);
	}


};