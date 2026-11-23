#include "Plant.h"

std::vector<float> Plant::getNoiseData(size_t size, float frequency, float time)
{
	std::vector<float> data(size * size);

	for (int x{}; x < size; ++x)
		for (int y{}; y < size; ++y)
		{
			static constexpr float placeHolder = 1.0f;
			data[y * size + x] = s_noiseGlobal.m_noise.compute(glm::vec4{ x,placeHolder,y,time });
		}

	return data;
}

GLuint Plant::generateNoiseTexture(size_t size, float frequency, float time)
{
	auto data = getNoiseData(size, frequency, time);
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

	return tex;
}

void Plant::updateNoiseTexture(size_t size, float frequency, float time)
{
	auto data = getNoiseData(size, frequency, time);
	glBindTexture(GL_TEXTURE_2D, s_noiseGlobal.m_noiseTexId);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size, size, GL_RED, GL_FLOAT, (GLfloat*)&data[0]);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Plant::destroy()
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

void Plant::refreshInstances()
{
	for (auto& component : m_components)
	{
		glBindVertexArray(component.vao);
		glBindBuffer(GL_ARRAY_BUFFER, component.instanceVbo);
		glBufferData(GL_ARRAY_BUFFER, m_instanceTransforms.size() * sizeof(glm::mat4), m_instanceTransforms.data(), GL_DYNAMIC_DRAW);
	}
	m_instancesChanged = false;
}

std::vector<glm::mat4> Plant::packData(const std::vector<glm::mat4>& transforms, const std::vector<glm::vec3>& colors)
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

void Plant::initializeNoise()
{
	s_noiseGlobal.m_noiseTexId = generateNoiseTexture(NOISE_SIZE, 0.05f, s_noiseGlobal.m_time);
}

void Plant::bindNoise(const char* name, size_t slot, std::weak_ptr<Shader> shader)
{
	if (auto s = shader.lock())
	{
		s->bindTexture(name, slot, s_noiseGlobal.m_noiseTexId);
	}
}

void Plant::setTransform(glm::mat4 transform)
{
	m_transform = transform;
}

size_t Plant::addComponent(Mesh mesh, glm::mat4 transform, Material material, std::weak_ptr<Shader> shader)
{
	m_components.emplace_back(mesh, transform, material, shader);
	return m_components.size() - 1;
}

void Plant::updateNoiseTexture()
{
	updateNoiseTexture(NOISE_SIZE, 0.05, s_noiseGlobal.m_time);
}

void Plant::addComponent(ModelComponent c)
{
	m_components.push_back(c);
}

void Plant::addInstanceColor(const glm::vec4& color)
{
	m_instanceColors.push_back(color);
}

void Plant::unitize()
{
	std::vector<std::reference_wrapper<Mesh>> meshList{};

	for (auto& c : m_components)
	{
		meshList.push_back(c.mesh);
	}

	Mesh::unitize(meshList);

}

void Plant::addInstanceTransform(glm::mat4 localTransform)
{
	m_instanceTransforms.push_back(localTransform);
	m_instancesChanged = true;
}

void Plant::arrangePlants(int rows, int cols, float spacing)
{
	for (int i = 0; i < rows; ++i)
	{
		for (int j = 0; j < cols; ++j)
		{
			const float x = spacing * j;
			const float z = spacing * i;

			glm::mat4 t = glm::translate(glm::mat4(1), glm::vec3(x, 0, z));
			addInstanceTransform(t);
		}
	}
}

void Plant::makeFractal(unsigned int density, unsigned int levels, float radius, float randomize, float rotFactor)
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

				float x = stackFrame.radius * angledUnitVectors[i].x + Random::nextFloat(-randomize, randomize);
				float z = stackFrame.radius * angledUnitVectors[i].y + Random::nextFloat(-randomize, randomize);

				glm::vec3 p = { x, 0, z };

				float phi = rotFactor * theta;

				glm::mat4 transform = translate(glm::mat4{ 1.f }, glm::vec3(x, 0, z)) * glm::rotate(glm::mat4(1), phi, glm::vec3(0, 1, 0));

				stack.emplace_back(StackFrame{ .t = transform * stackFrame.t, .levels = stackFrame.levels - 1, .radius = stackFrame.radius * 0.333f });
			}
		}
	}

	std::cout << this << " - Rendered instances: " << count << "\n";

}

void Plant::initialize()
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

void Plant::draw()
{

	if (m_instancesChanged)
	{
		refreshInstances();
	}
	for (auto& component : m_components)
	{
		if (auto lock = component.m_shader.lock())
		{

			lock->useShader();
			lock->setMat4("model", component.localTransform * m_transform);
			lock->setMaterial(component.material, false);
			lock->bindTexture("noiseTexture", 2, s_noiseGlobal.m_noiseTexId);
			glBindVertexArray(component.vao);
			glDrawElementsInstanced(GL_TRIANGLES, component.mesh.m_indices.size(), GL_UNSIGNED_INT, (void*)0, m_instanceTransforms.size());
		}
	}
}
