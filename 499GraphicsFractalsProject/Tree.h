#pragma once
#include "Branch.h"

class Tree
{
private:
	std::vector<Branch> branches{};
	GLuint m_res;
	GLfloat m_noise;

	void construct(glm::mat4 local, GLuint res, GLfloat noise, size_t n, GLfloat deltaX, GLfloat deltaY, GLfloat girth, GLfloat stride)
	{
		// Base radius set to 1.0f so girth directly controls the scale
		Branch branch{ 1.0f, static_cast<int>(res) };

		for (size_t s = 0; s < n; ++s)
		{
			GLfloat g = girth - girth * s / (GLfloat)n * 0.5f;
			GLfloat st = stride - stride * s / (GLfloat)n * 0.25f;

			branch.addLayer(local *
				glm::scale(glm::mat4(1.0f), glm::vec3(g, g, st)) *
				glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -0.5f)) *
				glm::rotate(glm::mat4(1.0f), glm::pi<GLfloat>() / 2.0f, glm::vec3(1.0f, 0.0f, 0.0f)), noise, 0);

			if (s > n / 2 && Random::nextFloat(0, 1) < 0.75f)
			{
				GLfloat theta = Random::nextFloat(0, 1) * glm::pi<GLfloat>();
				construct(local *
					glm::rotate(glm::mat4(1.0f), glm::pi<GLfloat>() / 2.0f, glm::vec3(1.0f, 0.0f, 0.0f)) *
					glm::rotate(glm::mat4(1.0f), theta, glm::vec3(0.0f, 1.0f, 0.0f)), res, noise, n / 2, -deltaX, deltaY, g * 0.5f, st);
			}

			local *= glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -st));
			local *= glm::rotate(glm::mat4(1.0f), deltaX * 0.1f, glm::vec3(1.0f, 0.0f, 0.0f));
			local *= glm::rotate(glm::mat4(1.0f), deltaY * 0.05f, glm::vec3(0.0f, 1.0f, 0.0f));

			deltaX += (Random::nextFloat(0, 1) - Random::nextFloat(0, 1)) * Random::nextFloat(0, 1) * 2.0f;
			deltaY += (Random::nextFloat(0, 1) - Random::nextFloat(0, 1)) * Random::nextFloat(0, 1) * 4.0f;
		}

		if (n > 1) {
			GLfloat theta = Random::nextFloat(0, 1) * glm::pi<GLfloat>();
			construct(local *
				glm::rotate(glm::mat4(1.0f), glm::pi<GLfloat>() / 4.0f, glm::vec3(1.0f, 0.0f, 0.0f)), res, noise, n * 2 / 3, -deltaX, deltaY, girth * 0.5f, stride * 0.75f);
			construct(local *
				glm::rotate(glm::mat4(1.0f), -glm::pi<GLfloat>() / 4.0f, glm::vec3(1.0f, 0.0f, 0.0f)), res, noise, n * 2 / 3, -deltaX, deltaY, girth * 0.5f, stride * 0.75f);
		}

		branches.push_back(std::move(branch));
	}

public:
	Tree(GLuint res, GLfloat noise)
		: m_res(res), m_noise(noise)
	{
	}

	void reconstruct(glm::mat4 local, size_t n, GLfloat deltaX, GLfloat deltaY, GLfloat girth, GLfloat stride)
	{
		branches.clear();
		construct(local, m_res, m_noise, n, deltaX, deltaY, girth, stride);
		for (auto& b : branches)
		{
			b.initialize();
		}
	}

	void draw()
	{
		for (auto& b : branches)
		{
			b.draw();
		}
	}
};