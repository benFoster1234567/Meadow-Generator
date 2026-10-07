#pragma once
#include "Mesh.h"
#include "Random.h"
//#include "OpenGL.h"

#include <stack>
class Branch
{
private:

	GLuint m_vao{};
	GLuint m_vbo{};
	GLuint m_ebo{};
	GLuint m_instancedBo{};
	GLfloat m_girth{.1f};

	size_t level = 1;
	size_t m_layerRes = 4;

	std::vector<glm::vec2> m_layerShape{};
	std::vector<glm::vec2> m_layerNorms{};

	std::vector<VertexData> m_data{};
	std::vector<unsigned int> m_indices{};

	size_t m_layerCount{0};

	std::stack<glm::mat4> layerTransforms{};

	glm::mat4 m_lastTransform{};
	std::vector<glm::vec2> m_lastOffset{};

	void addLayerHelper(const glm::mat4& transform, std::vector<glm::vec2> offsets);
	void subdivideLayerArgs(std::vector<glm::mat4>& transforms, std::vector<std::vector<glm::vec2>>& offsets, int layerSize);
	std::vector<glm::vec3> m_instancePositions{};

public:
	Branch(float radius, int res)
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
		addLayer(1.0f);
	}

	Branch(float side)
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
		addLayer(glm::mat4{ 1.0f });
	}

	Branch(Branch&& other) noexcept
		: m_data(std::exchange(other.m_data, {}))
		, m_indices(std::exchange(other.m_indices, {}))
		, m_vbo(std::exchange(other.m_vbo, 0))
		, m_vao(std::exchange(other.m_vao, 0))
		, m_ebo(std::exchange(other.m_ebo, 0))
	{
	}

	Branch(const Branch&) = delete;
	Branch& operator=(const Branch&) = delete;

	~Branch()
	{
		glDeleteBuffers(1, &m_vbo);
		m_vbo = 0;
		glDeleteBuffers(1, &m_ebo);
		m_ebo = 0;
		glDeleteVertexArrays(1, &m_vao);
		m_vao = 0;
	}

	void addLayer(const glm::mat4& transform, float noise = 0.f, unsigned int subdiv = 0);

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

	void draw();


};