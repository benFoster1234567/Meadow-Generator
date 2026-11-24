#pragma once
#include "Noise.h"

#include <GL/glew.h>

class AnimatedNoise
{
public:
	GLuint generateNoiseTexture(size_t size, float frequency)
	{
		m_size = size;
		m_freq = frequency;
		auto data = getNoiseData(size, frequency, 0);
		GLuint tex = 0;
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // one row of 
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, static_cast<GLsizei>(size), static_cast<GLsizei>(size), 0, GL_RED, GL_FLOAT, (GLfloat*)&data[0]);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glBindTexture(GL_TEXTURE_2D, 0);
		m_tex = tex;
		return tex;
	}

	void updateNoiseTexture(float deltaTime)
	{
		m_time += deltaTime;
		auto data = getNoiseData(m_size, m_freq, m_time);
		glBindTexture(GL_TEXTURE_2D, m_tex);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_size, m_size, GL_RED, GL_FLOAT, (GLfloat*)&data[0]);
		glBindTexture(GL_TEXTURE_2D, 0);
	}


private:
	inline static Noise4D s_perlin;
	size_t m_size{};
	float m_freq{};
	float m_time{ 0.f };
	GLuint m_tex{0};

	std::vector<float> getNoiseData(size_t size, float frequency, float time)
	{
		std::vector<float> data(size * size);

		for (int x{}; x < size; ++x)
			for (int y{}; y < size; ++y)
			{
				static constexpr float placeHolder = 1.0f;
				data[y * size + x] = s_perlin.compute(glm::vec4{ x,placeHolder,y,time });
			}

		return data;
	}
};