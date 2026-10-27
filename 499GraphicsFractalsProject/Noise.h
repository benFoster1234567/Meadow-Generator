#pragma once
#include <array>
#include <numeric>
#include <algorithm>
#include <random>
#include <glm/glm.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/common.hpp>
#include <glm/gtx/dual_quaternion.hpp>
#include <fstream>

class Noise4D
{

	std::array<short int, 256> p{};
	std::random_device rd;
	std::mt19937 g;

	static constexpr std::array<glm::vec4, 16> gradients4D
	{
		glm::ivec4{ 1, 1, 1, 1},
		glm::ivec4{ 1, 1, 1,-1},
		glm::ivec4{ 1, 1,-1, 1},
		glm::ivec4{ 1, 1,-1,-1},
		glm::ivec4{ 1,-1, 1, 1},
		glm::ivec4{ 1,-1, 1,-1},
		glm::ivec4{ 1,-1,-1, 1},
		glm::ivec4{ 1,-1,-1,-1},
		glm::ivec4{-1, 1, 1, 1},
		glm::ivec4{-1, 1, 1,-1},
		glm::ivec4{-1, 1,-1, 1},
		glm::ivec4{-1, 1,-1,-1},
		glm::ivec4{-1,-1, 1, 1},
		glm::ivec4{-1,-1, 1,-1},
		glm::ivec4{-1,-1,-1, 1},
		glm::ivec4{-1,-1,-1,-1}
	};

	float fade(float t)
	{
		return t * t * t * (t * (t * 6 - 15) + 10);
	}

	float lerp(float a, float b, float factor)
	{
		return a + factor * (b - a);
	}

	glm::vec4 gradient(const glm::ivec4& target)
	{
		int h = p[p[p[p[target.x & 255] + target.y & 255] + target.z & 255] + target.w & 255];
		return gradients4D[h & 15];
	}

public:

	Noise4D()
	{
		g = std::mt19937(rd());
		std::iota(p.begin(), p.end(), 0);
		std::shuffle(p.begin(), p.end(), g);
	}

	float compute(const glm::vec4& p)
	{
		using namespace glm;
		ivec4 p0 = glm::floor(p);

		std::array<float, 16> influence{};

		constexpr std::array<ivec4, 16> corners =
		{
			ivec4{0,0,0,0}, ivec4{0,0,0,1}, ivec4{0,0,1,0}, ivec4{0,0,1,1},
			ivec4{0,1,0,0}, ivec4{0,1,0,1}, ivec4{0,1,1,0}, ivec4{0,1,1,1},
			ivec4{1,0,0,0}, ivec4{1,0,0,1}, ivec4{1,0,1,0}, ivec4{1,0,1,1},
			ivec4{1,1,0,0}, ivec4{1,1,0,1}, ivec4{1,1,1,0}, ivec4{1,1,1,1}
		};

		for (int n{ 0 }; n < 16; ++n)
		{
			auto vecSource = corners[n] + p0;
			vec4 distance = p - vec4(vecSource);
			influence[n] = dot(gradient(vecSource), distance);
		}

		float u = fade(p.x - p0.x);
		float v = fade(p.y - p0.y);
		float w = fade(p.z - p0.z);
		float t = fade(p.w - p0.w);

		//interpolate x
		std::array<float, 8> wInterp{};
		for (int l{ 0 }; l < 8; ++l)
		{
			int h = l + 8;
			wInterp[l] = lerp(influence[l], influence[h], u);
		}

		//TODO: noise shifting along the y axis is broken and choppy.

		//interpolate y
		std::array<float, 4> zInterp{};
		for (int l{ 0 }; l < 4; ++l)
		{
			int h = l + 4;
			zInterp[l] = lerp(wInterp[l], wInterp[h], v);
		}

		//interpolate z
		std::array<float, 2> yInterp{};
		for (int l{ 0 }; l < 2; ++l)
		{
			int h = l + 2;
			yInterp[l] = lerp(zInterp[l], zInterp[h], w);
		}

		//interpolate w
		float result = lerp(zInterp[0], zInterp[1], t);
		//return result;
		return std::clamp(result * 0.5f + 0.5f, 0.0f, 1.0f);
	}

	void savebmp(const char* filename, int wisth, int h, int dpi, float z, float w)
	{
		FILE* f;
		int k = wisth * h;
		int s = 4 * k;
		int filesize = 54 + s;

		double factor = 39.375;
		int m = static_cast<int>(factor);
		int ppm = dpi * m;

		unsigned char bmpfileheader[14] = { 'B', 'M',  0, 0, 0, 0,  0, 0, 0, 0,  54, 0, 0, 0 };
		unsigned char bmpinfoheader[40] = { 40, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1, 0, 24, 0 };

		bmpfileheader[2] = (unsigned char)(filesize);
		bmpfileheader[3] = (unsigned char)(filesize >> 8);
		bmpfileheader[4] = (unsigned char)(filesize >> 16);
		bmpfileheader[5] = (unsigned char)(filesize >> 24);

		bmpinfoheader[4] = (unsigned char)(wisth);
		bmpinfoheader[5] = (unsigned char)(wisth >> 8);
		bmpinfoheader[6] = (unsigned char)(wisth >> 16);
		bmpinfoheader[7] = (unsigned char)(wisth >> 24);

		bmpinfoheader[8] = (unsigned char)(h);
		bmpinfoheader[9] = (unsigned char)(h >> 8);
		bmpinfoheader[10] = (unsigned char)(h >> 16);
		bmpinfoheader[11] = (unsigned char)(h >> 24);

		bmpinfoheader[21] = (unsigned char)(s);
		bmpinfoheader[22] = (unsigned char)(s >> 8);
		bmpinfoheader[23] = (unsigned char)(s >> 16);
		bmpinfoheader[24] = (unsigned char)(s >> 24);

		bmpinfoheader[25] = (unsigned char)(ppm);
		bmpinfoheader[26] = (unsigned char)(ppm >> 8);
		bmpinfoheader[27] = (unsigned char)(ppm >> 16);
		bmpinfoheader[28] = (unsigned char)(ppm >> 24);

		bmpinfoheader[29] = (unsigned char)(ppm);
		bmpinfoheader[30] = (unsigned char)(ppm >> 8);
		bmpinfoheader[31] = (unsigned char)(ppm >> 16);
		bmpinfoheader[32] = (unsigned char)(ppm >> 24);

		fopen_s(&f, filename, "wb");
		fwrite(bmpfileheader, 1, 14, f);
		fwrite(bmpinfoheader, 1, 40, f);

		std::vector<float> data{};

		for (int i{}; i < wisth; ++i)
		{
			for (int j{}; j < h; ++j)
			{
				float x = static_cast<float>(i) / 20;
				float y = static_cast<float>(j) / 20;

				data.push_back(compute({ x,y,z, w }));
			}
		}

		for (int i = 0; i < data.size(); i++) {

			double red = (data[i]) * 255;
			double green = (data[i]) * 255;
			double blue = (data[i]) * 255;

			unsigned char color[3] = { (int)floor(blue), (int)floor(green), (int)floor(red) };

			fwrite(color, 1, 3, f);
		}
		fclose(f);
	}

};
