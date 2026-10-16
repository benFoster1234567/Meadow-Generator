#pragma once
#include <array>
#include <numeric>
#include <algorithm>
#include <random>
#include <glm/ext/vector_float2.hpp>
#include <glm/common.hpp>
#include <glm/gtx/dual_quaternion.hpp>
#include <fstream>

inline static size_t fastRoundUp(float f)
{
	return static_cast<size_t>(f + 0.9999999f);
}

inline static size_t fastRoundDown(float f)
{
	return static_cast<size_t>(f);
}

class Noise
{
	std::array<int, 256> p{};

	std::random_device rd;
	std::mt19937 g;

	float fade(float t)
	{
		return t * t * t * (t * (t * 6 - 15) + 10);
	}

	glm::vec2 gradient(float x, float y)
	{
		int h = p[p[static_cast<int>(x) & 255] + static_cast<int>(y) & 255];
		const float theta = static_cast<float>(h) / static_cast<float>(256) * 2 * glm::pi<float>();
		return { glm::cos(theta), glm::sin(theta) };
	}

	float lerp(float a, float b, float factor)
	{
		return a + factor * (b - a);
	}

public:
	Noise()
	{
		g = std::mt19937(rd());
		std::iota(p.begin(), p.end(), 0);
		std::shuffle(p.begin(), p.end(), g);
	}

	float getValue(glm::vec2 target)
	{
		float x = target.x
			, y = target.y
			, x0 = std::floor(target.x)
			, y0 = std::floor(target.y)
			, x1 = x0 + 1
			, y1 = y0 + 1;

		glm::vec2 LL = glm::vec2{ x,y } - glm::vec2{ x0, y0 };
		glm::vec2 LU = glm::vec2{ x,y } - glm::vec2{ x0, y1 };
		glm::vec2 UL = glm::vec2{ x,y } - glm::vec2{ x1, y0 };
		glm::vec2 UU = glm::vec2{ x,y } - glm::vec2{ x1, y1 };

		float TR = glm::dot(gradient(x1, y1), UU);
		float BR = glm::dot(gradient(x1, y0), UL);
		float TL = glm::dot(gradient(x0, y1), LU);
		float BL = glm::dot(gradient(x0, y0), LL);

		float u = fade(x - x0);
		float v = fade(y - y0);

		float left = lerp(BL, TL, v);
		float right = lerp(BR, TR, v);

		return lerp(left, right, u) * 0.5f + 0.5f;
	}

	void toPpm(float div)
	{
		std::ofstream stream{ "noiseTestOut.ppm" };

		struct {
			size_t w = 400;
			size_t h = 300;
		} noiseRender;

		stream << "P3\n" << noiseRender.w << " " << noiseRender.h << "\n255\n";
		for (int i{}; i < noiseRender.h; i++)
		{
			for (int j{}; j < noiseRender.w; j++)
			{
				auto noiseOut = getValue(glm::vec2(static_cast<float>(j) / div, static_cast<float>(i) / div));
				int r = noiseOut * 255;
				int g = noiseOut * 255;
				int b = noiseOut * 255;
				stream << r << " " << g << " " << b << " ";
			}
			stream << "\n";
		}

	}

};
