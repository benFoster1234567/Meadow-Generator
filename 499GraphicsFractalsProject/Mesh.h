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
	bool   m_modelLoaded{ false };

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

		//for (size_t i = 0; i + 2 < m_indices.size(); i += 3)
		//{
		//	std::swap(m_indices[i + 1], m_indices[i + 2]);
		//}

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


};