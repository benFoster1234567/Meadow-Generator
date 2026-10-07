#include "Branch.h"

void Branch::addLayerHelper(const glm::mat4& transform, std::vector<glm::vec2> offsets)
{
	++m_layerCount;

	layerTransforms.push(transform);

	//position, normal

	glm::vec2 offset0 = offsets[0];

	for (int i{}; i < m_layerShape.size(); i++)
	{
		glm::vec2 noiseOffset{};

		if (i == 0 || i == m_layerShape.size() - 1)
		{
			noiseOffset = offset0;
		}
		else
		{
			noiseOffset = offsets[i];
		}

		float px = m_layerShape[i].x + noiseOffset.x;
		float pz = m_layerShape[i].y + noiseOffset.y;

		float nx = m_layerNorms[i].x;
		float nz = m_layerNorms[i].y;

		glm::vec3 pos = transform * glm::vec4{ px, 0.f, pz, 1.f };
		glm::vec3 norm = glm::normalize(transform * glm::vec4{ nx, 0.f, nz, 0.f });

		VertexData vData{ pos, norm,{ 0,0 } };
		m_data.push_back(vData);
	}

	size_t layerSize = m_layerShape.size();
	size_t vertexCount = m_data.size();

	//indices
	if (m_layerCount > 1)
	{
		for (size_t i = vertexCount - layerSize; i < vertexCount; ++i)
		{
			size_t ul = i;
			size_t ur = i < vertexCount - 1 ? i + 1 : i - layerSize + 1;
			size_t dl = ul - layerSize;
			size_t dr = ur - layerSize;

			m_indices.push_back(ul);
			m_indices.push_back(dl);
			m_indices.push_back(ur);

			m_indices.push_back(ur);
			m_indices.push_back(dl);
			m_indices.push_back(dr);
		}
	}

	//texCoords
	{
		for (size_t layer{}; layer < m_layerCount; layer++)
		{
			for (size_t vertex{}; vertex < layerSize; vertex++)
			{
				size_t index = vertex + layer * layerSize;
				float x = static_cast<float>(vertex) / static_cast<float>(layerSize - 1);
				float y = (m_layerCount > 1) ?
					static_cast<float>(layer) / static_cast<float>(m_layerCount - 1) :
					0.0f;
				m_data[index].texCoord = { x, y };
			}
		}
	}
}

void Branch::subdivideLayerArgs(std::vector<glm::mat4>& transforms, std::vector<std::vector<glm::vec2>>& offsets, int layerSize)
{
	if (transforms.empty() || offsets.empty()) return;
	std::vector<glm::mat4> oldTransforms = (transforms);
	std::vector<std::vector<glm::vec2>> oldOffsets = (offsets);

	transforms.clear();
	offsets.clear();

	size_t count = std::min(oldTransforms.size(), oldOffsets.size());

	for (int i{}; i < oldTransforms.size() - 1; i++)
	{
		{
			const auto& cur = oldTransforms[i];
			const auto& next = oldTransforms[i + 1];

			glm::mat4 interpTransform = (cur * 0.5f) + (next * 0.5f);

			transforms.push_back(cur);
			transforms.push_back(interpTransform);
		}

		{
			const auto& cur = oldOffsets[i];
			const auto& next = oldOffsets[i + 1];

			std::vector<glm::vec2> interpOffset{};

			for (int j{}; j < cur.size(); j++)
			{
				interpOffset.push_back(glm::mix(cur[j], next[j], 0.5f));
			}
			offsets.push_back(cur);
			offsets.push_back(interpOffset);
		}
	}

	transforms.push_back(oldTransforms[count - 1]);
	offsets.push_back(oldOffsets[count - 1]);
}

void Branch::addLayer(const glm::mat4& transform, float noise, unsigned int subdiv)
{
	std::vector<glm::mat4> transforms{};
	transforms.push_back(m_lastTransform);
	transforms.push_back(transform);
	m_lastTransform = transform;

	std::vector<glm::vec2> newOffsets{};
	std::vector<glm::vec2> oldOffsets = m_lastOffset;

	for (int i = 0; i < m_layerShape.size();++i)
	{
		newOffsets.push_back(glm::vec2{ Random::nextFloat() * noise, Random::nextFloat() * noise });
	}

	std::vector<std::vector<glm::vec2>> offsetSets{};
	offsetSets.push_back(oldOffsets);
	offsetSets.push_back(newOffsets);
	m_lastOffset = newOffsets;

	for (int i = 0; i < subdiv; i++)
	{
		subdivideLayerArgs(transforms, offsetSets, m_layerShape.size());
	}

	for (int i = 1; i < transforms.size(); i++)
	{
		addLayerHelper(transforms[i], offsetSets[i]);
	}


}

void Branch::draw()
{
	glBindVertexArray(m_vao);
	glDrawElements(GL_TRIANGLES, m_indices.size(), GL_UNSIGNED_INT, (void*)0);
}
