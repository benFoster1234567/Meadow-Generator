#pragma once

#include "SparseSet.h"
#include <glm/ext/matrix_float4x4.hpp>
#include <algorithm>
#include <glm/trigonometric.hpp>

class Fractals
{
	using TreeIndex = size_t;

	struct BezierQuad
	{
		glm::vec3 m_p0;
		glm::vec3 m_p1;
		glm::vec3 m_p2;

		glm::vec3 computeP1() 
		{
			//TODO
			return glm::vec3(0);
		}

		BezierQuad(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2)
			: m_p0{p0}, m_p1(p1), m_p2{p2}
		{}

		glm::vec3 b(float t)
		{
			t = std::clamp(t, 0.f, 1.f);
			return (1 - t) * ((1 - t) * m_p0 + t * m_p1) + t * ((1 - t) * m_p1 + t * m_p2);
		}
	};

	struct TreeNode
	{
		TreeIndex index;
		glm::vec3 position;
		std::vector<TreeIndex> children;
	};

	int lastVisited = -1;
	//TODO: find a better way to pack this or ensure it's cache local?
	SparseSet<TreeNode> transformTree{};



	void construct(glm::mat4 local, float noise, size_t n, float deltaX, float deltaY, float girth, float stride)
	{
		
	}

};