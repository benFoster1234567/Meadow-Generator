#pragma once
#include <memory>
#include <utility>

#include "GpuMesh.h"
#include "Shader.h"
#include "Transform.h"

class RenderedObject
{
private:
	std::shared_ptr<GpuMesh> m_mesh{ nullptr };
	std::weak_ptr<Shader> m_shader{};
	Material m_material{};
	Transform m_transform;
	size_t m_rigidbodyID{ 0 };

public:

	RenderedObject(std::shared_ptr<GpuMesh> mesh, std::weak_ptr<Shader> shader, Material material)
		: m_mesh(std::move(mesh)), m_shader(std::move(shader)), m_material(std::move(material))
	{
	}

	void attachRigidbody(const size_t& id) { m_rigidbodyID = id; }
	const size_t& getRigidbodyId() { return m_rigidbodyID; }

	void initialize()
	{
		if (!m_mesh->isInitialized()) m_mesh->initialize();
	}

	const Material& getMaterial() const { return m_material; }
	Material& getMaterial() { return m_material; }

	bool hasShader() { return !m_shader.expired(); }

	Transform& getTransform()
	{
		return m_transform;
	}

	void draw(bool sendMatMaps)
	{
		auto shader = m_shader.lock();

		if (!shader || !m_mesh) return;

		shader->setMaterial(m_material, sendMatMaps);
		shader->setMat4("model", m_transform.matrix());
		m_mesh->draw();
	}

	const std::shared_ptr<GpuMesh>& getMesh()
	{
		return m_mesh;
	}
};
