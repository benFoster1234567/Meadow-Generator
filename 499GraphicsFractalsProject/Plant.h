#pragma once
#include "InstancedMesh.h"

#define OLD_PLANT
//#define NEW_PLANT

#ifdef NEW_PLANT
class Plants
{

	// needs shader programs for the different components

	InstancedMesh flowerStem{ Mesh{ "stem.obj" } };
	InstancedMesh flowerPetals{ Mesh{"petals.obj"} };
	InstancedMesh flowerLeaf1{ Mesh{"leaf1.obj"} };
	InstancedMesh flowerLeaf1{ Mesh{"leaf2.obj"} };
	InstancedMesh flowerLeaf1{ Mesh{"leaf3.obj"} };

	

public:

	void initializePlants()
	{

	}




};
#endif

#ifdef OLD_PLANT


#include "Mesh.h"
#include "Random.h"
#include "OpenGL.h"
#include <queue>


//plants are made up of components, which have their own instance colors.
struct ModelComponent
{
	GLuint vao{ 0 };
	GLuint vbo{ 0 };
	GLuint ebo{ 0 };
	GLuint instanceColorVbo{ 0 };
	GLuint instanceVbo{ 0 };

	GLuint program{ 0 };

	Mesh mesh;
	
	glm::mat4 localTransform{ 1.f };
	
	Material material{ { 0.2f, 0.2f, 0.2f }, { 0.5f, 0.5f, 0.5f }, { 1.f, 1.f, 1.f }, 32.f, 0, 0 };

	std::vector<glm::vec3> colors{};

	ModelComponent(Mesh m, glm::mat4 t, Material material, GLuint p)
		: mesh(std::move(m)), localTransform(t), material(material), program(p)
	{
	}

};

class Plant
{
	bool m_initialized{ false };
	bool m_instancesChanged{ false };

	std::vector<ModelComponent> m_components{};
	std::vector<glm::mat4> m_instanceTransforms{};
	std::vector<glm::vec4> m_instanceColors{};

	glm::mat4 m_transform{1.f};

	void destroy()
	{
		for (auto& c : m_components)
		{
			glDeleteBuffers(1, &c.vbo);
			glDeleteBuffers(1, &c.ebo);
			glDeleteBuffers(1, &c.instanceVbo);
			glDeleteVertexArrays(1, &c.vao);
		}
		m_components.clear();
		m_instanceTransforms.clear();
		m_initialized = false;
	}

	void refreshInstances()
	{
		for (auto& component : m_components)
		{
			glBindVertexArray(component.vao);
			glBindBuffer(GL_ARRAY_BUFFER, component.instanceVbo);
			glBufferData(GL_ARRAY_BUFFER, m_instanceTransforms.size() * sizeof(glm::mat4), m_instanceTransforms.data(), GL_DYNAMIC_DRAW);
		}
		m_instancesChanged = false;
	}

	std::vector<glm::mat4> packData(const std::vector<glm::mat4>& transforms, const std::vector<glm::vec3>& colors)
	{
		auto combine = [](const glm::vec3& c, const glm::mat4& t) -> glm::mat4
			{
				glm::mat4 m = t;
				m[0].w = c.r;
				m[1].w = c.g;
				m[2].w = c.b;
				return m;
			};

		std::vector<glm::mat4> packed;
		packed.reserve(transforms.size());

		if (colors.empty())
		{
			for (const auto& t : transforms)
				packed.push_back(combine(glm::vec3(1.0f), t));
			return packed;
		}

		size_t interval = 1;
		if (transforms.size() > colors.size())
		{
			interval = transforms.size() / colors.size();
		}

		for (size_t i = 0; i < transforms.size(); ++i)
		{
			size_t colorIndex = std::min(i / interval, colors.size() - 1);
			packed.push_back(combine(colors[colorIndex], transforms[i]));
		}

		return packed;
	}

public:

	Plant() = default;
	Plant(const Plant&) = delete;
	Plant& operator=(const Plant&) = delete;
	Plant(Plant&& other) noexcept
		: m_components(std::move(other.m_components)),
		m_instanceTransforms(std::move(other.m_instanceTransforms)),
		m_initialized(other.m_initialized),
		m_instancesChanged(other.m_instancesChanged)
	{
	}

	Plant& operator=(Plant&& other) noexcept
	{
		if (this != &other)
		{
			m_components = std::move(other.m_components);
			m_instanceTransforms = std::move(other.m_instanceTransforms);
			m_initialized = other.m_initialized;
			m_instancesChanged = other.m_instancesChanged;
		}
		return *this;
	}

	~Plant()
	{
		destroy();
	}

	void setTransform(glm::mat4 transform)
	{
		m_transform = transform;
	}

	size_t addComponent(Mesh mesh, glm::mat4 transform, Material material, GLuint program)
	{
		m_components.emplace_back(mesh, transform, material, program);
		return m_components.size() - 1;
	}

	void addComponent(ModelComponent c)
	{
		m_components.push_back(c);
	}

	void addInstanceColor(const glm::vec4& color)
	{
		m_instanceColors.push_back(color);
	}

	void addInstanceTransform(glm::mat4 localTransform)
	{
		m_instanceTransforms.push_back(localTransform);
		m_instancesChanged = true;
	}

	void makeFractal(unsigned int density, unsigned int levels, float radius, float randomize = 0.3f, float rotFactor = 1.0f)
	{
		glm::mat4 t{ 1.f };
		size_t count = 0;
		struct StackFrame
		{
			glm::mat4 t;
			unsigned int levels;
			float radius;
		};

		std::vector<StackFrame> stack{};
		stack.reserve(10000000);
		stack.push_back(StackFrame{ .t = t,.levels = levels, .radius = radius });

		float interval = 2 * glm::pi<float>() / density;
		float r = 2 * glm::pi<float>() / Random::nextFloat(0, randomize);

		std::vector<glm::vec2> angledUnitVectors{};
		angledUnitVectors.reserve(density);

		for (int i{ 0 }; i < density; ++i)
		{
			float theta = i * interval + r;
			angledUnitVectors.emplace_back(glm::cos(theta), glm::sin(theta));
		}

		while (!stack.empty())
		{
			StackFrame stackFrame = stack.back();
			stack.pop_back();

			if (stackFrame.levels > 0)
			{
				addInstanceTransform(stackFrame.t);
				count++;
				for (int i = 0; i < density; ++i)
				{
					float theta = i * interval + r;

					float x = stackFrame.radius * angledUnitVectors[i].x + Random::nextFloat(-randomize, randomize); //replace with something way faster
					float z = stackFrame.radius * angledUnitVectors[i].y + Random::nextFloat(-randomize, randomize);

					glm::vec3 p = { x, 0, z };

					float phi = rotFactor * theta;

					glm::mat4 transform = translate(glm::mat4{ 1.f }, glm::vec3(x, 0, z)) * glm::rotate(glm::mat4(1), phi, glm::vec3(0,1,0));

					stack.emplace_back(StackFrame{ .t = transform * stackFrame.t, .levels = stackFrame.levels - 1, .radius = stackFrame.radius * 0.333f });
				}
			}
		}

		std::cout << this << " - Rendered instances: " << count << "\n";

	}

	void initialize()
	{

		for (auto& component : m_components)
		{
			glGenVertexArrays(1, &component.vao);
			glBindVertexArray(component.vao);
			glGenBuffers(1, &component.vbo);
			glGenBuffers(1, &component.ebo);
			glGenBuffers(1, &component.instanceColorVbo);
			glGenBuffers(1, &component.instanceVbo);

			glBindBuffer(GL_ARRAY_BUFFER, component.vbo);
			glBufferData(GL_ARRAY_BUFFER, component.mesh.m_data.size() * sizeof(VertexData), component.mesh.m_data.data(), GL_STATIC_DRAW);

			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, point));
			glEnableVertexAttribArray(0);

			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, normal));
			glEnableVertexAttribArray(1);

			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, texCoord));
			glEnableVertexAttribArray(2);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, component.ebo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, component.mesh.m_indices.size() * sizeof(unsigned int), component.mesh.m_indices.data(), GL_STATIC_DRAW);

			if (component.colors.empty())
			{
				component.colors.push_back(component.material.diffuse);
			}
			std::vector<glm::mat4> packed = packData(m_instanceTransforms, component.colors); // if I share the array among all components, I wouldn't have to copy and it would possibly still work?

			glBindBuffer(GL_ARRAY_BUFFER, component.instanceVbo);
			glBufferData(GL_ARRAY_BUFFER, packed.size() * sizeof(glm::mat4), packed.data(), GL_DYNAMIC_DRAW);

			for (int i = 0; i < 4; ++i) // puts the whole matrix into the shaders as 4 vec4s
			{
				GLuint attribIndex = 3 + i;
				GLuint stride = sizeof(glm::mat4);
				GLuint size = 4;
				glVertexAttribPointer(attribIndex, size, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(glm::vec4) * i));
				glEnableVertexAttribArray(attribIndex);
				glVertexAttribDivisor(attribIndex, 1);
			}
			m_instancesChanged = false;

			glBindVertexArray(0);
		}

		m_initialized = true;
	}

	void draw()
	{
		if (m_instancesChanged)
		{
			refreshInstances();
		}
		for (auto& component : m_components)
		{
			glUseProgram(component.program);
			setMat4("model", component.localTransform * m_transform, component.program);
			setMaterial(component.material, component.program);
			glBindVertexArray(component.vao);
			glDrawElementsInstanced(GL_TRIANGLES, component.mesh.m_indices.size(), GL_UNSIGNED_INT, (void*)0, m_instanceTransforms.size());
		}
	}

};

#endif