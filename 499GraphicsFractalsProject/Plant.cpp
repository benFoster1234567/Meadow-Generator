#include "Plant.h"
#include "Renderer.h"
#include "Renderable.h"

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

void Plant::setTransform(glm::mat4 transform)
{
	m_transform = transform;
}

size_t Plant::addComponent(MeshLoaderID mesh, const glm::mat4& transform, Material material, RenderPassID passID)
{
	m_components.emplace_back(mesh, transform, material, passID);
	return m_components.size() - 1;
}

void Plant::addComponent(ModelComponent c)
{
	m_components.push_back(c);
}

void Plant::addInstanceColor(const glm::vec4& color)
{
	m_instanceColors.push_back(color);
}


void Plant::addInstanceTransform(glm::mat4 localTransform)
{
	m_instanceTransforms.push_back(localTransform);
	m_instancesChanged = true;
}

void Plant::arrangePlantsInRows(int rows, int cols, float spacing)
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

void Plant::arrangePlantsUsingFractal(unsigned int density, unsigned int levels, float radius, float randomize, float rotFactor)
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
			//append the transform to the instance container
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
		if (component.colors.empty())
		{
			component.colors.push_back(component.m_material.diffuse);
		}

		auto packed = packData(m_instanceTransforms, component.colors); //very slow
		m_renderer.changeInstances(component.m_meshID, packed);

		Renderable renderable
		{
			.mesh = component.m_meshID,
			.isInstanced = true,
			.hasRigidbody = false,
			.transform = component.m_localTransform,
			.material = component.m_material
		};

		auto renderableID = m_renderer.addRenderable(std::move(renderable));
		m_renderer.attachRenderableToRenderPass(component.m_renderPass, renderableID);
	}
}
