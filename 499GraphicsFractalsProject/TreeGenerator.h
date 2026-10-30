#pragma once
#include "Mesh.h"
#include <stack>
#include "SparseSet.h"
#include "Noise.h"
#include <optional>

struct VertexLayer
{
	int id;
	int indexStart;
	int indexCount;
};

class TreeGenerator
{

private:
	std::vector<glm::vec2> m_layerShape{};
	std::vector<glm::vec2> m_layerNorms{};
	std::vector<glm::mat4> m_leafInstances{};
	SparseSet<VertexLayer> layers{};
	std::vector<VertexData> m_data{};
	std::vector<unsigned int> m_indices{};

	float m_time{};

	int nextId = 0;
	bool m_noiseEnabled{ true };
	Noise4D m_noise{};

	void addVertsHelper(const glm::mat4& transform, float noise, float v)
	{
		const size_t shapeCount = m_layerShape.size();
		const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(transform)));

		m_data.reserve(m_data.size() + shapeCount);

		for (size_t j = 0; j <= shapeCount; ++j)
		{
			const size_t i = j % shapeCount;

			const float px = m_layerShape[i].x;
			const float pz = m_layerShape[i].y;

			glm::vec2 offset{ 0.f };
			if (m_noiseEnabled)
			{
				constexpr float displacementScale = 0.5f;
				const glm::vec3 pos = glm::vec3(transform * glm::vec4(px * 0.001f, 0.f, pz * 0.001f, 1.f));
				offset = glm::vec2{
					m_noise.compute({ pos, noise }),
					m_noise.compute({ pos, noise + 10.f })
				} * displacementScale;
			}

			const glm::vec3 pos = glm::vec3(transform * glm::vec4(px + offset.x, 0.f, pz + offset.y, 1.f));
			const glm::vec3 norm = glm::normalize(normalMatrix * glm::vec3(-m_layerNorms[i].x, 0.f, -m_layerNorms[i].y));

			float u = 1 - (static_cast<float>(j) / static_cast<float>(shapeCount));
			m_data.push_back({ pos, norm, { u, v } });
		}
	}

	void addIndicesHelper(std::optional<VertexLayer> parent, VertexLayer layer)
	{
		if (!parent.has_value())
		{
			return;
		}

		const VertexLayer parentLayer = *parent;
		const size_t layerSize = m_layerShape.size();
		const size_t vertexCount = m_data.size();

		if (m_layerCount > 1)
		{
			for (size_t i = 0; i < layerSize; ++i)
			{
				const size_t ul = layer.indexStart + i;
				const size_t ur = (i < layerSize - 1) ? ul + 1 : layer.indexStart;
				const size_t dl = parentLayer.indexStart + i;
				const size_t dr = (i < layerSize - 1) ? dl + 1 : parentLayer.indexStart;

				m_indices.push_back(ul);
				m_indices.push_back(dl);
				m_indices.push_back(ur);

				m_indices.push_back(ur);
				m_indices.push_back(dl);
				m_indices.push_back(dr);


			}
		}
	}

	VertexLayer addLayer(std::optional<VertexLayer> parent, const glm::mat4& transform, float noise, float textureHeight)
	{
		//TODO: texture coords...
		if (parent.has_value())
		{
			VertexLayer parentLayer = *parent;
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
		};

		//layers.insert(layer.id, layer);
		nextId++;

		float height = transform[3].y;

		addVertsHelper(transform, noise, height / textureHeight);
		addIndicesHelper(parent, layer);
		m_layerCount++;

		return layer;
	}


	size_t m_layerCount{ 0 };

	std::stack<glm::mat4> layerTransforms{};

	glm::mat4 m_lastTransform{};
	std::vector<glm::vec2> m_lastOffset{};

	std::vector<glm::vec3> m_instancePositions{};

	glm::mat3 createLookatMatrix(const glm::vec3& targetUp)
	{
		using namespace glm;

		const vec3 forward = { 0,0,1 };

		const vec3 Y = targetUp;
		const vec3 X = normalize(cross(forward, Y));
		const vec3 Z = normalize(cross(X, Y));

		return mat3(X, Y, Z);
	}

	glm::mat3 createLookatMatrix(const glm::vec3& target, const glm::vec3& origin)
	{
		const glm::vec3 targetUp = glm::normalize(target - origin);
		return createLookatMatrix(targetUp);
	}

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

	std::vector<glm::mat4> getLeafInstances()
	{

		return m_leafInstances;
	}

	void generateTreeWithNoise(const glm::mat4& local, int levels, int n, float stride)
	{
		//local * up * Rnew?

		//using namespace glm;

		struct StackFrame
		{
			VertexLayer parent;
			glm::mat4 localRot;
			glm::mat4 localPos;
			float girth;
			float stride;
			int level;
			int n;
		};

		std::vector<StackFrame> stack{};

		float state = 0.0f;
		constexpr float stateDelta = 0.01f;
		constexpr float xPlane = 0.f;
		constexpr float yPlane = 20.f;
		constexpr float zPlane = 40.f;

		VertexLayer layer0 = addLayer(std::nullopt, local, 1, 10);

		stack.push_back({ .parent = layer0, .localRot = 1, .localPos = local, .girth = 1, .stride = 1,.level = levels,.n = n });

		float theta = 0.0f;
		float thetaDelta = 45.0f;

		constexpr std::array<glm::vec3, 4> dir =
		{
			glm::vec3{ 1,  1.0f, 1},
			glm::vec3{ -1, 1.0f, -1},
			glm::vec3{ -1, 1.0f, 1},
			glm::vec3{ 1,  1.0f, -1}
		};

		glm::vec3 axisRot{};
		while (!stack.empty())
		{

			state += stateDelta;
			theta += thetaDelta;

			StackFrame sf = stack.back();
			stack.pop_back();

			VertexLayer parent = addLayer(sf.parent, sf.localRot * sf.localPos, 1, 10);

			glm::mat4 translateM(1);

			for (int s{}; s < sf.n; ++s)
			{
				translateM = glm::translate(glm::mat4(translateM), glm::vec3(0, stride, 0));

				parent = addLayer(parent, sf.localRot * translateM, 1, 10);

				if (s > sf.n * 2 / 3)
				{
					//branch out 

					for (const auto& d : dir)
					{
						glm::mat4 rot = sf.localRot * glm::mat4(createLookatMatrix(d));
						stack.push_back(StackFrame{ .parent = parent, .localRot = rot, .localPos = sf.localRot * translateM * rot,.girth = 1, .stride = 1 });
					}
				}
			}
		}

	}

	void generateExampleTree(glm::mat4 local, int levels, int n, glm::vec3 scale, float deltaX, float deltaZ, float stride, float noiseScale, float textureHeight)
	{
		size_t vertexCount = 0;
		VertexLayer parent = addLayer(std::nullopt, local * glm::scale(glm::mat4(1), scale), noiseScale, textureHeight);

		if (levels <= 0)
		{
			return;
		}

		struct StackFrame
		{
			VertexLayer parent;
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

		std::vector<int> perLevel(levels + 1, 0);
		// in loop, after popping:

		while (!stack.empty())
		{
			StackFrame sf = stack.back();
			stack.pop_back();
			perLevel[sf.levels]++;

			if (sf.levels < 1)
			{
				continue;
			}

			sf.parent = addLayer(sf.parent, sf.local * glm::scale(glm::mat4(1), sf.scale), noiseScale, textureHeight);

			float radX = sf.stride / sf.deltaX;

			float theta = Random::nextFloat();


			float scaleFactor = 0.2f;
			for (int j{}; j < sf.n; ++j)
			{

				theta += glm::radians(360.f / static_cast<float>(sf.n));

				glm::mat4 tPivotInvX = glm::translate(glm::mat4(1), glm::vec3(-sf.deltaX, 0, 0));
				glm::mat4 tPivotInvZ = glm::translate(glm::mat4(1), glm::vec3(0, 0, -sf.deltaZ));
				glm::mat4 tPivotX = glm::translate(glm::mat4(1), glm::vec3(sf.deltaX, 0, 0));
				glm::mat4 tPivotZ = glm::translate(glm::mat4(1), glm::vec3(0, 0, sf.deltaZ));

				glm::mat4 rotX = glm::rotate(glm::mat4(1), radX, { 0,0,1 });
				glm::mat4 rotZ = glm::rotate(glm::mat4(1), radX, { 1,0,0 });

				glm::vec3 position = { sf.local[3] };
				float offsetX = m_noise.compute(glm::vec4(position, 0.f));
				float offsetY = m_noise.compute(glm::vec4(position, 10.f));

				glm::mat4 offsetMat = glm::translate(glm::mat4(1), glm::vec3{ offsetX, 0,offsetY });

				sf.local = sf.local * glm::translate(glm::mat4(1), glm::vec3(0, sf.stride, 0)) * (tPivotInvX * rotX * tPivotX);
				sf.local = sf.local * glm::translate(glm::mat4(1), glm::vec3(0, sf.stride, 0)) * (tPivotInvZ * rotZ * tPivotZ);


				sf.local = sf.local * offsetMat;
				// local = local * glm::translate(glm::mat4(1), glm::vec3(0, stride, 0)) * (tPivotInvX * rotX * tPivotX);

				// if (j == sf.n / 2) sf.deltaX = -sf.deltaX;

				float t = static_cast<float>(j + 1) / static_cast<float>(n + 1);

				scaleFactor = 0.2f + 1 / (10 * (2 * t * glm::sqrt(t) + 0.1f));

				constexpr float minGirth = 0.1f;

				glm::vec3 scaleTemp{};
				if (glm::length(sf.scale) < minGirth)
				{
					scaleTemp = sf.scale;
				}
				else
				{
					scaleTemp = sf.scale * scaleFactor;
				}

				//glm::vec3 scaleTemp = glm::vec3(scaleFactor * sf.scale.x, sf.scale.y, scaleFactor * sf.scale.z);
				int branchCount = sf.levels > levels * 4 / 5 ? 4 : 1;
				sf.parent = addLayer(sf.parent, sf.local * glm::scale(glm::mat4(1), scaleTemp), noiseScale, textureHeight);

				if (j > sf.n * 2 / 3)
				{
					float d = Random::nextFloat(-20.f, 45.f) + 90.f;
					theta += glm::radians(360.f / static_cast<float>(branchCount));

					glm::vec3 axis = glm::normalize(glm::vec3(Random::nextFloat(), 0, Random::nextFloat()));


					for (int i{}; i < 2; i++)
					{
						StackFrame s = { .parent = sf.parent, .local = sf.local * glm::rotate(glm::mat4(1), theta * (1 + i), {0,1,0}) * glm::rotate(glm::mat4(1), glm::radians(90.f), glm::vec3(1,0,0)),
							.levels = sf.levels - 1,
							.n = std::max(sf.n * 2 / 3, 1),
							.scale = scaleTemp,
							//.scale = sf.scale * scaleFactor,
							.deltaX = sf.deltaX * 2 / 3,
							.deltaZ = sf.deltaZ * 2 / 3 - 0.1f,
							.stride = sf.stride / 2
						};
						stack.push_back(s);
					}

				}

				if (j == sf.n - 1)
				{

					glm::vec3 axis = glm::normalize(glm::vec3(Random::nextFloat(), 0, Random::nextFloat()));

					for (int i{}; i < branchCount; i++)
					{
						float d = Random::nextFloat(-90.f, 90.f);
						StackFrame s = {
							.parent = sf.parent,
							.local = sf.local * glm::rotate(glm::mat4(1), glm::radians(d), axis),
							.levels = sf.levels - 1,
							.n = std::max(sf.n * 2 / 3, 1),
							.scale = scaleTemp,
							//.scale = sf.scale * scaleFactor,
							.deltaX = sf.deltaX / 2,
							.deltaZ = sf.deltaZ / 2,
							.stride = sf.stride / 2
						};
						stack.push_back(s);
					}

					if (sf.levels < 2 || sf.n <= 1)
					{
						m_leafInstances.push_back(sf.local);
					}
				}
			}

		}
	}

	Mesh getMesh()
	{
		Mesh m{};
		m.m_data = m_data;
		m.m_indices = m_indices;

		return m;
	}
};
