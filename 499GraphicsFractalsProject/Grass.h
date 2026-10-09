#pragma once

#include "Mesh.h"
#include "Random.h"

class Grass
{
	Mesh mesh{ Cube{}, 1.0f };

public:

	Grass() = default;
	Grass(const Grass&) = delete;
	Grass& operator=(const Grass&) = delete;
	Grass(Grass&& other) noexcept
		: mesh(std::move(other.mesh))
	{
	}

	Grass& operator=(Grass&& other) noexcept
	{
		if (this != &other)
		{
			mesh = std::move(other.mesh);
		}
		return *this;
	}

	void initialize()
	{
		mesh.initialize();
	}

	void draw()
	{
		mesh.draw();
	}

	void setDensity(size_t recursions, size_t density, float radius, glm::vec3 position = glm::vec3(0.f))
	{
		if (recursions == 0)
		{
			return;
		}

		float angle = Random::nextFloat(0.f, 2.f * glm::pi<float>());
		
		for (size_t root{}; root < density; ++root)
		{
			angle += 2.f * glm::pi<float>() / density;
			
			float x = radius * glm::cos(angle);
			float z = radius * glm::sin(angle);

			glm::vec3 instancePosition = position + glm::vec3(x, 0.f, z);
			mesh.addInstance(glm::translate(glm::mat4(1.f), instancePosition)*glm::rotate(glm::mat4(1.f), Random::nextFloat(0.f, 2.f * glm::pi<float>()), glm::vec3(0.f, 1.f, 0.f)));
			setDensity(recursions - 1, density, radius * 0.5f, instancePosition);

		}

	}
};