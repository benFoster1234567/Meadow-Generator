#pragma once
#include <random>

class Random
{
public:
	static float nextFloat(float min = -1.f, float max = 1.f)
	{

		static std::mt19937 generator;
		std::uniform_real_distribution<float> distribution(min, max);
		
		float dif = max - min;
		
		return dif == 0? 0.f : distribution(generator);
	}
};