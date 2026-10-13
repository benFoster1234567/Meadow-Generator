#pragma once
#include <vector>
#include <glm/ext/vector_float2.hpp>
#include <array>
#include <numeric>
#include <algorithm>
#include <random>
#include <glm/ext/vector_float3.hpp>
inline static size_t fastRoundUp(float f)
{
	return static_cast<size_t>(f + 0.9999999f);
}

inline static size_t fastRoundDown(float f)
{
	return static_cast<size_t>(f);
}

class PerlinNoise
{
	using PerlinIndex = size_t;
	size_t gridWidth = 1000;
	size_t gridHeight = 1000;
	float gridSize = 3.f;
	std::vector<glm::vec2> grid{};

	std::array<size_t, 256> permutation{};
	std::array<glm::vec2, 4> constantVectors{ glm::vec2{1.0f,1.0f}
											, glm::vec2{1.0f,-1.0f}
											, glm::vec2{-1.0f,-1.0f}
											, glm::vec2{-1.0f,1.0f} };

	std::random_device rd;
	std::mt19937 g;

	PerlinNoise()
	{
		g = std::mt19937(rd());
	}

	bool mathSearch(std::array<std::pair<size_t, size_t>, 4>& nearestCellsOut, const glm::vec2& target)
	{
		

		float x = target.x / gridSize;
		float x = target.y / gridSize;

		//find a way to get rid of this division

		size_t lowX =  fastRoundDown(target.x);
		size_t lowY =  fastRoundDown(target.y);
		size_t highX = fastRoundUp(target.x);
		size_t highY = fastRoundUp(target.y);

		//std::array<size_t, 4> roundedIndices = { fastRoundDown(target.x),fastRoundDown(target.y),fastRoundUp(target.x),fastRoundUp(target.y) };

		nearestCellsOut[0] = { lowX, lowY };
		nearestCellsOut[1] = { lowX, highY };
		nearestCellsOut[2] = { highX, highY };
		nearestCellsOut[3] = { highX, highY };

		//gets the indices nearest to the 
	}

	void initPermutationTable()
	{
		std::iota(std::begin(permutation), std::end(permutation), 0);
		std::shuffle(std::begin(permutation), std::end(permutation), g);
	}

	void gen2DNoise()
	{
		initPermutationTable();

		
	}

	double fade(double t) {
		return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
	}

	double noise(glm::vec3 v)
	{
		int x = (int)fastRoundDown(v.x) & 255;
		int y = (int)fastRoundDown(v.y) & 255;
		int z = (int)fastRoundDown(v.z) & 255;

		v.x -= static_cast<float>(fastRoundDown(v.x));
		v.y -= static_cast<float>(fastRoundDown(v.y));
		v.z -= static_cast<float>(fastRoundDown(v.z));

		double u = fade(x);
		double v = fade(y);
		double w = fade(z);

		int A = permutation[x] + y;
		int B = permutation[x + 1] + y;
		int AA = permutation[A] + z;
		int BA = permutation[B] + z;


	}
};