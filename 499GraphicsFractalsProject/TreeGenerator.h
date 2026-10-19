#pragma once
#include "Mesh.h"
#include <stack>
#include "SparseSet.h"
#include "Noise.h"

struct VertexLayer
{
	int id;
	int indexStart;
	int indexCount;

	int layerParent;

	int layerChild1;
	int layerChild2;
	int layerChild3;
};

class TreeGenerator
{

private:
	std::vector<glm::vec2> m_layerShape{};
	std::vector<glm::vec2> m_layerNorms{};
	SparseSet<VertexLayer> layers{};
	std::vector<VertexData> m_data{};
	std::vector<unsigned int> m_indices{};

	int nextId = 0;
	bool m_noiseEnabled{ false };
	Noise4D m_noise{};

	void addVertsHelper(int layerIdx, const glm::mat4& transform, float noise)
	{
		const size_t shapeCount = m_layerShape.size();
		const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(transform)));

		m_data.reserve(m_data.size() + shapeCount);

		for (size_t i = 0; i < shapeCount; ++i)
		{
			const float px = m_layerShape[i].x;
			const float pz = m_layerShape[i].y;

			glm::vec2 offset{ 0.f };
			if (m_noiseEnabled)
			{
				constexpr float displacementScale = 0.15f;
				const glm::vec3 pos = glm::vec3(transform * glm::vec4(px, 0.f, pz, 1.f));
				offset = glm::vec2{
					m_noise.compute({ pos, noise }),
					m_noise.compute({ pos, noise + 10.f })
				} * displacementScale;
			}

			const glm::vec3 pos = glm::vec3(transform * glm::vec4(px + offset.x, 0.f, pz + offset.y, 1.f));
			const glm::vec3 norm = glm::normalize(normalMatrix * glm::vec3(-m_layerNorms[i].x, 0.f, -m_layerNorms[i].y));

			m_data.push_back({ pos, norm, { 0.f, 0.f } });
		}
	}

	void addIndicesHelper(int parent, int layerIdx)
	{
		if (parent == -1)
		{
			return;
		}

		VertexLayer parentLayer = layers.get(parent);
		VertexLayer layer = layers.get(layerIdx);

		size_t layerSize = m_layerShape.size();
		size_t vertexCount = m_data.size();

		if (m_layerCount > 1)
		{
			for (size_t i = 0; i < layerSize; ++i)
			{
				size_t ul = layer.indexStart + i;
				size_t ur = (i < layerSize - 1) ? ul + 1 : layer.indexStart;
				size_t dl = parentLayer.indexStart + i;
				size_t dr = (i < layerSize - 1) ? dl + 1 : parentLayer.indexStart;

				m_indices.push_back(ul);
				m_indices.push_back(dl);
				m_indices.push_back(ur);

				m_indices.push_back(ur);
				m_indices.push_back(dl);
				m_indices.push_back(dr);
			}
		}
	}

	int addLayer(int parent, const glm::mat4& transform, float noise)
	{
		//TODO: texture coords...
		if (parent != -1)
		{
			VertexLayer parentLayer = layers.get(parent);
			if (m_layerShape.size() != parentLayer.indexCount)
			{
				throw std::runtime_error("Error creating tree mesh: parent indices do not align");
			}
		}

		int layerSize = m_layerShape.size();
		int vertexCount = m_data.size();

		VertexLayer layer
		{
			.id = nextId,
			.indexStart = vertexCount,
			.indexCount = layerSize,
			.layerParent = parent
		};

		layers.insert(layer.id, layer);
		nextId++;

		addVertsHelper(layer.id, transform, noise);
		addIndicesHelper(parent, layer.id);
		m_layerCount++;

		return layer.id;
	}

	std::pair<int, int> addConjuntion2(int parent, const glm::mat4& transform0, const glm::mat4& transform1, float noise)
	{
		// TODO: fix this whole thing here...
		return { addLayer(parent, transform0,noise), addLayer(parent, transform1, noise) };
	}

	std::tuple<int, int, int> addConjuntion3(int parent)
	{

	}
	

	size_t m_layerCount{ 0 };

	std::stack<glm::mat4> layerTransforms{};

	glm::mat4 m_lastTransform{};
	std::vector<glm::vec2> m_lastOffset{};

	void addLayerHelper(const glm::mat4& transform, std::vector<glm::vec2> offsets);
	void subdivideLayerArgs(std::vector<glm::mat4>& transforms, std::vector<std::vector<glm::vec2>>& offsets, int layerSize);
	std::vector<glm::vec3> m_instancePositions{};

public:
	TreeGenerator(float radius, int res)
	{
		for (int i = 0; i <= res; i++)
		{
			int index = i == res ? 0 : i;
			double angle = index * glm::pi<double>() * 2 / res;
			float x = radius * glm::cos(angle);
			float y = radius * glm::sin(angle);

			m_layerShape.push_back({ x, y });
			m_layerNorms.push_back(glm::normalize(glm::vec2{ x,y }));
		}
		m_lastOffset.resize(m_layerShape.size(), glm::vec2(0.0f));
		//addLayer(1.0f);
	}

	TreeGenerator(float side)
	{
		m_layerShape =
		{
			{side, side},
			{-side, side},
			{-side, -side},
			{side, -side},
		};

		m_layerNorms =
		{
			glm::normalize(glm::vec2(1,1)),
			glm::normalize(glm::vec2(-1,1)),
			glm::normalize(glm::vec2(-1,-1)),
			glm::normalize(glm::vec2(1,-1)),
		};
		m_lastOffset.resize(m_layerShape.size(), glm::vec2(0.0f));
	}

	Mesh genNewMesh(int deep, float noiseScale)
	{
		m_data.clear();
		m_indices.clear();

		int parent = -1;
		glm::mat4 transform = glm::mat4(1.0f);

		for (int i = 0; i < deep; ++i)
		{
			float t = static_cast<float>(i) / static_cast<float>(deep);
			float noiseX = (m_noise.compute(glm::vec4(0.0f, static_cast<float>(i), 0.0f, 0.0f)) * 2.0f - 1.0f) * noiseScale;
			float noiseZ = (m_noise.compute(glm::vec4(0.0f, static_cast<float>(i), 0.0f, 1.0f)) * 2.0f - 1.0f) * noiseScale;
			transform = glm::translate(transform, glm::vec3(noiseX, 1.0f, noiseZ));
			float radiusScale = 1.0f - (t * 0.8f);
			glm::mat4 layerTransform = glm::scale(transform, glm::vec3(radiusScale, 1.0f, radiusScale));
			parent = addLayer(parent, layerTransform, 1.f);
		}

		Mesh mesh;
		mesh.m_data = m_data;
		mesh.m_indices = m_indices;

		return mesh;
	}

};
