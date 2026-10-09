#pragma once
#include <GL/glew.h>
#include <vector>
#include <string>
#include <string_view>
#include <utility>
#include <cstddef>
#include <iostream>

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif

#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>   // translate, rotate, scale
#include <glm/gtx/hash.hpp>
#include <tiny_obj_loader.h>

struct VertexData
{
	glm::vec3 point{};
	glm::vec3 normal{};
	glm::vec2 texCoord{};

	VertexData() = default;

	VertexData(glm::vec3 p, glm::vec3 n, glm::vec2 tx)
		: point{ p }, normal{ n }, texCoord{ tx }
	{
	}
};

struct Cube {};
struct Sphere {};
struct Plane {};

class Mesh
{
public:
	std::vector<VertexData>   m_data{};
	std::vector<unsigned int> m_indices{};

private:

	static constexpr GLuint kInstanceAttrib = 3;

	bool   m_modelLoaded{ false };

	GLuint m_vao{};
	GLuint m_vbo{};
	GLuint m_ebo{};
	GLuint m_instancedBo{};

	std::vector<glm::mat4> m_instances{};
	size_t m_instanceCapacity{ 0 };
	bool   m_instancesDirty{ false };

	bool loadObjModel(std::string_view path)
	{
		if (m_modelLoaded)
			return false;

		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		std::string pathStr{ path };
		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, pathStr.c_str()))
		{
			std::cout << "Model mesh from " << path << " failed to load...\n";
			return false;
		}

		m_data.clear();
		m_indices.clear();

		const bool hasNormals = !attrib.normals.empty();
		const bool hasTexcoords = !attrib.texcoords.empty();

		for (const auto& shape : shapes)
		{
			for (const auto& index : shape.mesh.indices)
			{
				VertexData v{};
				v.point = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2] };

				if (hasNormals && index.normal_index >= 0)
				{
					v.normal = {
						attrib.normals[3 * index.normal_index + 0],
						attrib.normals[3 * index.normal_index + 1],
						attrib.normals[3 * index.normal_index + 2] };
				}
				else
				{
					v.normal = { 0.f, 1.f, 0.f };
				}

				if (hasTexcoords && index.texcoord_index >= 0)
				{
					v.texCoord = {
						attrib.texcoords[2 * index.texcoord_index + 0],
						attrib.texcoords[2 * index.texcoord_index + 1] };
				}
				else
				{
					v.texCoord = { 0.f, 0.f };
				}

				m_data.push_back(v);
				m_indices.push_back(static_cast<unsigned int>(m_data.size() - 1));
			}
		}

		m_modelLoaded = true;
		return true;
	}

	bool loadCubeModel(float side)
	{
		m_data.push_back(VertexData{ { side,  side,  side }, { 0.0f,  0.0f,  1.0f }, {1.0f, 1.0f} });
		m_data.push_back(VertexData{ {-side,  side,  side }, { 0.0f,  0.0f,  1.0f }, {0.0f, 1.0f} });
		m_data.push_back(VertexData{ {-side, -side,  side }, { 0.0f,  0.0f,  1.0f }, {0.0f, 0.0f} });
		m_data.push_back(VertexData{ { side, -side,  side }, { 0.0f,  0.0f,  1.0f }, {1.0f, 0.0f} });
		m_data.push_back(VertexData{ { side,  side,  side }, { 1.0f,  0.0f,  0.0f }, {0.0f, 1.0f} });
		m_data.push_back(VertexData{ { side, -side,  side }, { 1.0f,  0.0f,  0.0f }, {0.0f, 0.0f} });
		m_data.push_back(VertexData{ { side, -side, -side }, { 1.0f,  0.0f,  0.0f }, {1.0f, 0.0f} });
		m_data.push_back(VertexData{ { side,  side, -side }, { 1.0f,  0.0f,  0.0f }, {1.0f, 1.0f} });
		m_data.push_back(VertexData{ { side,  side,  side }, { 0.0f,  1.0f,  0.0f }, {0.0f, 1.0f} });
		m_data.push_back(VertexData{ { side,  side, -side }, { 0.0f,  1.0f,  0.0f }, {0.0f, 0.0f} });
		m_data.push_back(VertexData{ {-side,  side, -side }, { 0.0f,  1.0f,  0.0f }, {1.0f, 0.0f} });
		m_data.push_back(VertexData{ {-side,  side,  side }, { 0.0f,  1.0f,  0.0f }, {1.0f, 1.0f} });
		m_data.push_back(VertexData{ {-side,  side,  side }, {-1.0f,  0.0f,  0.0f }, {0.0f, 1.0f} });
		m_data.push_back(VertexData{ {-side,  side, -side }, {-1.0f,  0.0f,  0.0f }, {1.0f, 1.0f} });
		m_data.push_back(VertexData{ {-side, -side, -side }, {-1.0f,  0.0f,  0.0f }, {1.0f, 0.0f} });
		m_data.push_back(VertexData{ {-side, -side,  side }, {-1.0f,  0.0f,  0.0f }, {0.0f, 0.0f} });
		m_data.push_back(VertexData{ {-side, -side, -side }, { 0.0f, -1.0f,  0.0f }, {1.0f, 0.0f} });
		m_data.push_back(VertexData{ { side, -side, -side }, { 0.0f, -1.0f,  0.0f }, {0.0f, 0.0f} });
		m_data.push_back(VertexData{ { side, -side,  side }, { 0.0f, -1.0f,  0.0f }, {0.0f, 1.0f} });
		m_data.push_back(VertexData{ {-side, -side,  side }, { 0.0f, -1.0f,  0.0f }, {1.0f, 1.0f} });
		m_data.push_back(VertexData{ { side, -side, -side }, { 0.0f,  0.0f, -1.0f }, {0.0f, 0.0f} });
		m_data.push_back(VertexData{ {-side, -side, -side }, { 0.0f,  0.0f, -1.0f }, {1.0f, 0.0f} });
		m_data.push_back(VertexData{ {-side,  side, -side }, { 0.0f,  0.0f, -1.0f }, {1.0f, 1.0f} });
		m_data.push_back(VertexData{ { side,  side, -side }, { 0.0f,  0.0f, -1.0f }, {0.0f, 1.0f} });

		m_indices = {
			 0,  1,  2,  2,  3,  0,
			 4,  5,  6,  6,  7,  4,
			 8,  9, 10, 10, 11,  8,
			12, 13, 14, 14, 15, 12,
			16, 17, 18, 18, 19, 16,
			20, 21, 22, 22, 23, 20
		};

		m_modelLoaded = true;
		return true;
	}

	void loadPlaneModel(float size)
	{
		m_data.push_back(VertexData{ { -size, 0.0f,  size }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } });
		m_data.push_back(VertexData{ {  size, 0.0f,  size }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f } });
		m_data.push_back(VertexData{ { -size, 0.0f, -size }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } });
		m_data.push_back(VertexData{ {  size, 0.0f, -size }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } });

		m_indices = {
			0, 1, 2,
			3, 2, 1
		};

		m_modelLoaded = true;
	}

	void destroy() noexcept
	{
		glDeleteBuffers(1, &m_vbo);
		glDeleteBuffers(1, &m_ebo);
		glDeleteBuffers(1, &m_instancedBo);
		glDeleteVertexArrays(1, &m_vao);
		m_vbo = m_ebo = m_instancedBo = m_vao = 0;
		m_instanceCapacity = 0;
	}

	void uploadInstances()
	{
		glBindVertexArray(m_vao);
		glBindBuffer(GL_ARRAY_BUFFER, m_instancedBo);

		if (m_instances.empty())
		{
			for (GLuint i = 0; i < 4; ++i)
			{
				glDisableVertexAttribArray(kInstanceAttrib + i);
				glVertexAttrib4f(kInstanceAttrib + i,
					i == 0 ? 1.f : 0.f,
					i == 1 ? 1.f : 0.f,
					i == 2 ? 1.f : 0.f,
					i == 3 ? 1.f : 0.f);
			}
		}
		else
		{
			const size_t count = m_instances.size();
			const GLsizeiptr bytes = static_cast<GLsizeiptr>(count * sizeof(glm::mat4));

			if (count > m_instanceCapacity)
			{
				glBufferData(GL_ARRAY_BUFFER, bytes, m_instances.data(), GL_DYNAMIC_DRAW);
				m_instanceCapacity = count;
			}
			else
			{
				glBufferSubData(GL_ARRAY_BUFFER, 0, bytes, m_instances.data());
			}

			for (GLuint i = 0; i < 4; ++i)
				glEnableVertexAttribArray(kInstanceAttrib + i);
		}

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
		m_instancesDirty = false;
	}

public:
	Mesh() = default;

	explicit Mesh(std::string_view path)
	{
		loadObjModel(path);
	}

	Mesh(Cube, float side)
	{
		loadCubeModel(side);
	}

	Mesh(Plane, float size)
	{
		loadPlaneModel(size);
	}

	Mesh(const Mesh&) = delete;
	Mesh& operator=(const Mesh&) = delete;

	Mesh(Mesh&& other) noexcept
		: m_data(std::exchange(other.m_data, {}))
		, m_indices(std::exchange(other.m_indices, {}))
		, m_modelLoaded(other.m_modelLoaded)
		, m_vao(std::exchange(other.m_vao, 0))
		, m_vbo(std::exchange(other.m_vbo, 0))
		, m_ebo(std::exchange(other.m_ebo, 0))
		, m_instancedBo(std::exchange(other.m_instancedBo, 0))
		, m_instances(std::exchange(other.m_instances, {}))
		, m_instanceCapacity(std::exchange(other.m_instanceCapacity, 0))
		, m_instancesDirty(std::exchange(other.m_instancesDirty, false))
	{
	}

	Mesh& operator=(Mesh&& other) noexcept
	{
		if (this != &other)
		{
			destroy();
			m_data = std::exchange(other.m_data, {});
			m_indices = std::exchange(other.m_indices, {});
			m_modelLoaded = other.m_modelLoaded;
			m_vao = std::exchange(other.m_vao, 0);
			m_vbo = std::exchange(other.m_vbo, 0);
			m_ebo = std::exchange(other.m_ebo, 0);
			m_instancedBo = std::exchange(other.m_instancedBo, 0);
			m_instances = std::exchange(other.m_instances, {});
			m_instanceCapacity = std::exchange(other.m_instanceCapacity, 0);
			m_instancesDirty = std::exchange(other.m_instancesDirty, false);
		}
		return *this;
	}

	~Mesh() { destroy(); }

	void initialize()
	{
		destroy(); 
		glGenVertexArrays(1, &m_vao);
		glGenBuffers(1, &m_vbo);
		glGenBuffers(1, &m_ebo);
		glGenBuffers(1, &m_instancedBo);

		glBindVertexArray(m_vao);

		glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
		glBufferData(GL_ARRAY_BUFFER, m_data.size() * sizeof(VertexData), m_data.data(), GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(unsigned int), m_indices.data(), GL_STATIC_DRAW);

		const GLsizei stride = sizeof(VertexData);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(VertexData, point));

		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(VertexData, normal));

		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(VertexData, texCoord));

		glBindBuffer(GL_ARRAY_BUFFER, m_instancedBo);
		for (GLuint i = 0; i < 4; ++i)
		{
			glVertexAttribPointer(kInstanceAttrib + i, 4, GL_FLOAT, GL_FALSE,
				sizeof(glm::mat4), (void*)(i * sizeof(glm::vec4)));
			glVertexAttribDivisor(kInstanceAttrib + i, 1);
		}

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		m_instancesDirty = true;
	}

	void addInstance(const glm::mat4& transform)
	{
		m_instances.push_back(transform);
		m_instancesDirty = true;
	}

	void addInstance(glm::vec3 position,
		glm::vec3 scale = glm::vec3(1.f),
		glm::vec3 rotationAxis = glm::vec3(0.f, 1.f, 0.f),
		float angleRadians = 0.f)
	{
		glm::mat4 m = glm::translate(glm::mat4(1.f), position);
		if (angleRadians != 0.f)
			m = glm::rotate(m, angleRadians, rotationAxis);
		m = glm::scale(m, scale);
		addInstance(m);
	}

	void setInstances(std::vector<glm::mat4> transforms)
	{
		m_instances = std::move(transforms);
		m_instancesDirty = true;
	}

	void clearInstances()
	{
		m_instances.clear();
		m_instancesDirty = true;
	}

	size_t instanceCount() const { return m_instances.size(); }

	void draw()
	{
		if (m_instancesDirty)
			uploadInstances();

		glBindVertexArray(m_vao);

		if (m_instances.empty())
		{
			glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indices.size()),
				GL_UNSIGNED_INT, (void*)0);
		}
		else
		{
			glDrawElementsInstanced(GL_TRIANGLES, static_cast<GLsizei>(m_indices.size()),
				GL_UNSIGNED_INT, (void*)0, static_cast<GLsizei>(m_instances.size()));
		}
	}
};