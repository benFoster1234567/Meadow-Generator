#pragma once

#include "SparseSet.h"
#include <glm/ext/matrix_float4x4.hpp>

class Fractals
{
	struct TreeNode
	{
		int idx{};
		std::vector<int> children;
	};

	int lastVisited = -1;
	//TODO: find a better way to pack this or ensure it's cache local?
	std::vector<glm::mat4> transforms{};
	SparseSet<TreeNode> transformTree{};



	SparseSet<std::vector<glm::mat4>> generateTree()
	{

	}

	void construct(glm::mat4 local, float noise, size_t n, float deltaX, float deltaY, float girth, float stride)
	{
		
	}

};