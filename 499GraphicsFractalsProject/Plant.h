#pragma once
#include "MeshLoaderInstanced.h"
#include <glm/gtx/transform2.hpp>
#define OLD_PLANT
//#define NEW_PLANT

#ifdef OLD_PLANT


#include "Mesh.h"
#include "Random.h"
#include "Noise.h"
#include "Shader.h"

//plants are made up of components, which have their own instance colors.
struct ModelComponent
{
	GLuint vao{ 0 };
	GLuint vbo{ 0 };
	GLuint ebo{ 0 };
	GLuint instanceColorVbo{ 0 };
	GLuint instanceVbo{ 0 };

	std::weak_ptr<Shader> m_shader;

	Mesh mesh;

	glm::mat4 localTransform{ 1.f };

	Material material{ { 0.2f, 0.2f, 0.2f }, { 0.5f, 0.5f, 0.5f }, { 1.f, 1.f, 1.f }, 32.f, 0, 0 };

	std::vector<glm::vec3> colors{};

	ModelComponent(Mesh m, glm::mat4 t, Material material, std::weak_ptr<Shader> shader)
		: mesh(std::move(m)), localTransform(t), material(material), m_shader(shader)
	{
	}

};


class Plant
{
	static constexpr size_t NOISE_SIZE = 16;

	bool m_initialized{ false };
	bool m_instancesChanged{ false };

	inline static struct
	{
		Noise4D m_noise{};
		GLuint m_noiseTexId{};
		float m_time{ 0.f };
	}s_noiseGlobal{}; //makes sense to be static since we only really need it from one source?


	std::vector<ModelComponent> m_components{};
	std::vector<glm::mat4> m_instanceTransforms{};
	std::vector<glm::vec4> m_instanceColors{};

	glm::mat4 m_transform{ 1.f };


	static std::vector<float> getNoiseData(size_t size, float frequency, float time);


	static GLuint generateNoiseTexture(size_t size, float frequency, float time);

	static void updateNoiseTexture(size_t size, float frequency, float time);

	void destroy();

	void refreshInstances();


	std::vector<glm::mat4> packData(const std::vector<glm::mat4>& transforms, const std::vector<glm::vec3>& colors);

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

	static void initializeNoise();

	static void tick(float deltaTime)
	{
		s_noiseGlobal.m_time += deltaTime;
	}

	void bindNoise(const char* name, size_t slot, std::weak_ptr<Shader> shader);

	void setTransform(glm::mat4 transform);

	size_t addComponent(Mesh mesh, glm::mat4 transform, Material material, std::weak_ptr<Shader> shader);

	static void updateNoiseTexture();

	void addComponent(ModelComponent c);

	void addInstanceColor(const glm::vec4& color);

	void unitize();

	void addInstanceTransform(glm::mat4 localTransform);

	void arrangePlants(int rows, int cols, float spacing);

	void makeFractal(unsigned int density, unsigned int levels, float radius, float randomize = 0.3f, float rotFactor = 1.0f);

	void initialize();

	void draw();

};

#endif
