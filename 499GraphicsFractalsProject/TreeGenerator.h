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
				} * noise;
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



	int generateBranch(int parent, glm::mat4 local, int levels, int n, glm::vec3 scale, float deltaX, float deltaZ, float stride, float noiseScale)
	{
		parent = addLayer(parent, local * glm::scale(glm::mat4(1), scale), noiseScale);

		if (levels <= 0)
		{
			return parent;
		}

		struct StackFrame
		{
			int parent;
			glm::mat4 local;
			int levels;
			int n;
			glm::vec3 scale;
			float deltaX;
			float deltaZ;
			float stride;
		};

		std::vector<StackFrame> stack{};

		{
			StackFrame s = {
				.parent = parent,
				.local = local,
				.levels = levels,
				.n = n,
				.scale = scale,
				.deltaX = deltaX,
				.deltaZ = deltaZ,
				.stride = stride
			};
			stack.push_back(s);
		}

		while (!stack.empty())
		{
			StackFrame sf = stack.back();
			stack.pop_back();

			if (sf.levels < 1)
			{
				continue;
			}


			sf.parent = addLayer(sf.parent, sf.local * glm::scale(glm::mat4(1), sf.scale), noiseScale);

			float radX = sf.stride / sf.deltaX;

			float theta = 0.f;

			float scaleFactor = 0.1f;
			for (int i{}; i < sf.n; ++i)
			{
				theta += glm::radians(360.f / static_cast<float>(n));

				glm::mat4 tPivotInvX = glm::translate(glm::mat4(1), glm::vec3(-sf.deltaX, 0, 0));
				glm::mat4 tPivotInvZ = glm::translate(glm::mat4(1), glm::vec3(0, 0, -sf.deltaZ));
				glm::mat4 tPivotX = glm::translate(glm::mat4(1), glm::vec3(sf.deltaX, 0, 0));
				glm::mat4 tPivotZ = glm::translate(glm::mat4(1), glm::vec3(0, 0, sf.deltaZ));

				glm::mat4 rotX = glm::rotate(glm::mat4(1), radX, { 0,0,1 });
				glm::mat4 rotZ = glm::rotate(glm::mat4(1), radX, { 1,0,0 });

				sf.local = sf.local * glm::translate(glm::mat4(1), glm::vec3(0, sf.stride, 0)) * (tPivotInvX * rotX * tPivotX);
				sf.local = sf.local * glm::translate(glm::mat4(1), glm::vec3(0, sf.stride, 0)) * (tPivotInvZ * rotZ * tPivotZ);
				//local = local * glm::translate(glm::mat4(1), glm::vec3(0, stride, 0)) * (tPivotInvX * rotX * tPivotX);

				float t = static_cast<float>(i + 1) / static_cast<float>(n + 1);
				scaleFactor = 0.2f + 1 / (10 * (2 * t * glm::sqrt(t) + 0.1f));

				glm::vec3 scaleTemp = glm::vec3(scaleFactor * sf.scale.x, sf.scale.y, scaleFactor * sf.scale.z);

				sf.parent = addLayer(sf.parent, sf.local * glm::scale(glm::mat4(1), sf.scale * scaleFactor), noiseScale);

				if (i > sf.n * 2 / 3)
				{
					float d = Random::nextFloat(-20.f, 45.f) + 90.f;
					theta += glm::radians(360.f / static_cast<float>(n));

					glm::vec3 axis = glm::normalize(glm::vec3(Random::nextFloat(), 0, Random::nextFloat()));

					for (int i{}; i < 2; i++)
					{
						StackFrame s = {
							.parent = sf.parent,
							.local = sf.local * glm::rotate(glm::mat4(1), theta, {0,1,0}) * glm::rotate(glm::mat4(1), glm::radians(90.f), glm::vec3(1,0,0)),
							.levels = sf.levels - 1,
							.n = sf.n / 2,
							.scale = sf.scale * scaleFactor,
							.deltaX = sf.deltaX * 2 / 3,
							.deltaZ = sf.deltaZ * -2 / 3,
							.stride = sf.stride * 2 / 3
						};
						stack.push_back(s);
					}

				}

				if (i == sf.n - 1)
				{
					float d = Random::nextFloat(-90.f, 90.f);

					glm::vec3 axis = glm::normalize(glm::vec3(Random::nextFloat(), 0, Random::nextFloat()));

					for (int i{}; i < 2; i++)
					{
						StackFrame s = {
							.parent = sf.parent,
							.local = sf.local * glm::rotate(glm::mat4(1), glm::radians(d), axis),
							.levels = sf.levels - 1,
							.n = sf.n / 2,
							.scale = sf.scale * scaleFactor,
							.deltaX = sf.deltaX / 2,
							.deltaZ = -sf.deltaZ / 2,
							.stride = sf.stride * 2 / 3
						};
						stack.push_back(s);
					}

					//generateBranch(parent, local * glm::rotate(glm::mat4(1), glm::radians(d), axis), (levels - 1) / 2, n / 2, scale * scaleFactor, deltaX / 2, deltaZ / 2, stride * 2 / 3, noiseScale);
				}
			}
		}

		std::cout << "layers recorded: " << layers.size() << "\n";


		return parent;
	}

	Mesh getMesh()
	{
		Mesh m{};
		m.m_data = m_data;
		m.m_indices = m_indices;

		return m;
	}
};
