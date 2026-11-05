#pragma once
#include <memory>
#include <utility>

#include "GpuMesh.h"
#include "Shader.h"
#include "Transform.h"

enum class RenderFlag : uint8_t
{
	None = 0,
	DontSendMat = 1 << 0,
	DontSendMaps = 1 << 1
};

class RenderedObject
{
private:
	std::shared_ptr<GpuMesh> m_mesh{ nullptr };
	std::weak_ptr<Shader> m_shader{};
	Material m_material{};
	Transform m_transform{};
	size_t m_rigidbodyID{ 0 };

public:
	RenderedObject() = default;
	RenderedObject(std::weak_ptr<Shader> shader, Material material)
		: m_mesh(std::make_shared<GpuMesh>()), m_shader(std::move(shader)), m_material(std::move(material))
	{
	}

	RenderedObject(const Mesh& mesh, std::weak_ptr<Shader> shader, Material material)
		: m_mesh(std::make_shared<GpuMesh>(mesh)), m_shader(std::move(shader)), m_material(std::move(material))
	{
	}

	RenderedObject(std::shared_ptr<GpuMesh> mesh, std::weak_ptr<Shader> shader, Material material)
		: m_mesh(std::move(mesh)), m_shader(std::move(shader)), m_material(std::move(material))
	{
	}



	void attachRigidbody(const size_t& id) { m_rigidbodyID = id; }
	const size_t& getRigidbodyId() { return m_rigidbodyID; }

	void initialize()
	{
		m_mesh->initialize();
		//if (!m_mesh->isInitialized()) m_mesh->initialize();
	}

	const Material& getMaterial() const { return m_material; }
	Material& getMaterial() { return m_material; }

	bool hasShader() { return !m_shader.expired(); }

	Transform& getTransform()
	{
		return m_transform;
	}

	void draw(uint8_t flags = 0)
	{
		auto shader = m_shader.lock();
		if (!shader || !m_mesh) return;

		shader->useShader();

		bool dontSendMatMaps = (flags & static_cast<uint8_t>(RenderFlag::DontSendMaps)) != 0;
		bool dontSendMat = (flags & static_cast<uint8_t>(RenderFlag::DontSendMat)) != 0;

		if (!dontSendMat)
		{
			shader->setMaterial(m_material, !dontSendMatMaps);
		}

		shader->setMat4("model", m_transform.matrix());
		m_mesh->draw();
	}

	void draw(std::shared_ptr<Shader>& altShader)
	{
		altShader->useShader();
		altShader->setMat4("model", m_transform.matrix());
		m_mesh->draw();
	}

	const std::shared_ptr<GpuMesh>& getMesh()
	{
		return m_mesh;
	}
};
