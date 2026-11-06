#pragma once
#include "Shader.h"
#include "GpuMesh.h"
#include "Physics.h"
#include "RenderedObject.h"
#include "PickingTexture.h"
#include "Camera.h"

#include "Collider.h"

#include <deque>


class TreeItemPicker
{
	bool m_initialized = false;
	float m_mass{ 1.f };

	std::weak_ptr<Shader> m_shader{};
	std::shared_ptr<Shader> m_pickerShader{};
	std::shared_ptr<GpuMesh> m_mesh{};

	Material m_material{};
	Collider m_collider;

	bool m_enabled{true};

	glm::vec3 m_scale{1};

	BtPhysicsEngine m_physics{};
	PickingTexture m_pickingTexture{};

	struct TreeDangler
	{
		RenderedObject m_object;
		bool isFree;
	};
	std::deque<TreeDangler> m_objects{};

	TreeDangler addBox(const glm::mat4& transform, const glm::vec3& halfBounds, float mass);
	TreeDangler addItem(const glm::mat4& transform, const Collider& halfBounds, float mass);

	inline void changeRigidbodyPosition(int objIndex, glm::mat4 matrix, size_t id)
	{
		Transform t{};
		t.set(matrix);
		m_physics.setRigidbodyTransform(t, id);
		m_objects[objIndex].m_object.getTransform().set(matrix);
	}

	void freeObject(const unsigned int& objectId);
	void draw(const Camera& camera, std::weak_ptr<Shader> shader, uint8_t flags, bool toPicker);

public:

	TreeItemPicker() = default;

	void initialize(unsigned int width, unsigned int height,
		std::shared_ptr<GpuMesh> mesh,
		Material material,
		std::shared_ptr<Shader> shader,
		std::shared_ptr<Shader> pickerShader,
		const std::vector<glm::mat4>& initialTransforms,
		const Collider& collider,
		float mass);

	void draw(const Camera& camera)
	{
		draw(camera, m_shader, 0, false);
	}

	void changePositions(const std::vector<glm::mat4>& transforms);

	void onClick(int x, int y, const  Camera& camera);

	void onResize(int w, int h)
	{
		m_pickingTexture.resize(w, h);
	}

	void enable()
	{
		m_enabled = true;
	}

	void disable()
	{
		m_enabled = false;
	}

	void setScale(const glm::vec3& scale)
	{
		m_scale = scale;
	}

	void createKinematicRigidbody(const glm::vec3& position, const glm::vec3& halfExtents)
	{
		Transform t{};
		t.setPosition(position);
		m_physics.createRigidBodyBox(halfExtents, t, 0, true, true);
	}

	void runSim(float deltaTime)
	{
		if (!m_enabled) return;
		m_physics.runSimulation(deltaTime);
		for (auto& obj : m_objects)
		{
			if (obj.isFree)
			{
				Transform t;
				m_physics.getRigidbodyTransform(&t, obj.m_object.getRigidbodyId());
				obj.m_object.getTransform().set(t.matrix() * glm::scale(glm::mat4(1.f), m_scale));
			}
		}
	}

};