#pragma once
#include <GL/glew.h>
#include <vector>

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif

#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/gtx/hash.hpp>
#include <string_view>
#include <tiny_obj_loader.h>
#include <iostream>
#include <glm/ext/matrix_float4x4.hpp>



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
private:
	bool m_modelLoaded{ false };
	
	GLuint m_vao;
	GLuint m_vbo;
	GLuint m_ebo;
	GLuint m_instancedBo{};

	bool loadObjModel(std::string_view path)
	{
		if (m_modelLoaded)
		{
			return false;
		}

		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.data())) {
			std::cout << "Model mesh from " << path << " failed to load...\n";
			return false;
		}

		m_data.clear();

		bool hasNormals = !attrib.normals.empty();
		bool hasTexcoords = !attrib.texcoords.empty();

		for (const auto& shape : shapes)
		{
			for (const auto& index : shape.mesh.indices)
			{
				VertexData vData{};
				vData.point.x = attrib.vertices[3 * index.vertex_index + 0];
				vData.point.y = attrib.vertices[3 * index.vertex_index + 1];
				vData.point.z = attrib.vertices[3 * index.vertex_index + 2];

				if (hasNormals && index.normal_index >= 0)
				{
					vData.normal.x = attrib.normals[3 * index.normal_index + 0];
					vData.normal.y = attrib.normals[3 * index.normal_index + 1];
					vData.normal.z = attrib.normals[3 * index.normal_index + 2];
				}

				else
				{
					vData.normal = { 0.f,1.f,0.f };
				}
				if (hasTexcoords && index.texcoord_index >= 0)
				{
					vData.texCoord.x = attrib.texcoords[2 * index.texcoord_index + 0];
					vData.texCoord.y = attrib.texcoords[2 * index.texcoord_index + 1];
				}
				else
				{
					vData.texCoord = { 0,0 };
				}

				m_data.push_back(vData);
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
		m_data.push_back(VertexData{ { -size, 0.0f, -size }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } });
		m_data.push_back(VertexData{ {  size, 0.0f,  size }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f } });
		
		m_indices = {
			0, 1, 2,
			3, 2, 1
		};

		m_modelLoaded = true;
	}

public:
	std::vector<VertexData> m_data{};
	std::vector<unsigned int> m_indices{};

	std::vector<glm::vec3> m_instancePositions{};

	Mesh() = default;

	Mesh(std::string_view path)
	{
		loadObjModel(path);
		//deduplicateVertices();
	}

	Mesh(Cube, float side) noexcept
	{
		loadCubeModel(side);
	}

	Mesh(Plane, float size) noexcept
	{
		loadPlaneModel(size);
	}

	Mesh(Mesh&& other) noexcept
		: m_modelLoaded(other.m_modelLoaded)
		, m_data(std::exchange(other.m_data, {}))
		, m_indices(std::exchange(other.m_indices, {}))
		, m_vbo(std::exchange(other.m_vbo, 0))
		, m_vao(std::exchange(other.m_vao, 0))
		, m_ebo(std::exchange(other.m_ebo, 0))
	{
	}

	Mesh(const Mesh&) = delete;
	Mesh& operator=(const Mesh&) = delete;

	~Mesh()
	{
		glDeleteBuffers(1, &m_vbo);
		m_vbo = 0;
		glDeleteBuffers(1, &m_ebo);
		m_ebo = 0;
		glDeleteVertexArrays(1, &m_vao);
		m_vao = 0;
	}

	void initialize()
	{
		glGenVertexArrays(1, &m_vao);
		glGenBuffers(1, &m_vbo);
		glGenBuffers(1, &m_ebo);

		glBindVertexArray(m_vao);

		glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
		glBufferData(GL_ARRAY_BUFFER, m_data.size() * sizeof(VertexData), m_data.data(), GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(unsigned int), m_indices.data(), GL_STATIC_DRAW);

		GLsizei stride = sizeof(VertexData);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(VertexData, point));

		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(VertexData, normal));

		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(VertexData, texCoord));

		glBindVertexArray(0);
	}

	void addInstance(glm::vec3 position)
	{
		m_instancePositions.push_back(position);
	}

	void draw()
	{
		glBindVertexArray(m_vao);
		glDrawElements(GL_TRIANGLES, m_indices.size(), GL_UNSIGNED_INT, (void*)0 );
	}


};