#pragma once
#include "Shader.h"
#include "GpuMesh.h"
#include "Physics.h"
#include "RenderedObject.h"
#include "PickingTexture.h"
#include "Camera.h"

#include <deque>

class TreeItemPicker
{
	bool m_initialized = false;

	float m_mass{ 1.f };

	std::weak_ptr<Shader> m_shader{};
	std::shared_ptr<Shader> m_pickerShader{};
	std::shared_ptr<GpuMesh> m_mesh{};

	Material m_material{};

	glm::vec3 m_halfBounds{};

	BtPhysicsEngine m_physics{};

	PickingTexture m_pickingTexture{};

	struct TreeDangler
	{
		RenderedObject m_object;
		bool isFree;
	};
	std::deque<TreeDangler> m_objects{};

	TreeDangler addBox(const glm::mat4& transform, const glm::vec3& halfBounds, float mass)
	{
		Transform t{};
		t.set(transform);

		RenderedObject obj{ m_mesh, m_shader, m_material };
		auto rigidBody = m_physics.createRigidBodyBox(halfBounds, t, mass, false, false);
		obj.attachRigidbody(rigidBody);

		TreeDangler d = { .m_object = obj, .isFree = false };

		return d;
	}

	inline void changeRigidbodyPosition(int objIndex, glm::mat4 matrix, size_t id)
	{
		Transform t{};
		t.set(matrix);
		m_physics.setRigidbodyTransform(t, id);
		m_objects[objIndex].m_object.getTransform().set(matrix);
	}

	void changePositions(const std::vector<glm::mat4>& transforms, const glm::vec3& halfBounds)
	{
		constexpr int MaxRemainingObjects = 10;
		std::deque<TreeDangler> newObjects{};

		int count = 0;
		for (int i = 0; i < transforms.size(); ++i)
		{
			if (i >= m_objects.size())
			{
				const auto& d = addBox(transforms[i], halfBounds, m_mass);
				newObjects.push_back(d);
			}
			else if (m_objects[i].isFree)
			{
				count++;
				newObjects.push_front(m_objects[i]);
			}
			else
			{
				auto rib = m_objects[i].m_object.getRigidbodyId();
				changeRigidbodyPosition(i, transforms[i], rib);
			}
		}

		for (int i = 0; i < count - MaxRemainingObjects; ++i)
		{
			newObjects.pop_back();
		}

		m_objects = std::move(newObjects);
	}

	void freeObject(const unsigned int& objectId)
	{
		if (m_objects[objectId].isFree) return;

		m_objects[objectId].isFree = true;
		auto rib = m_objects[objectId].m_object.getRigidbodyId();
		m_physics.activateRigidbody(rib);
	}

	void draw(const Camera& camera, std::weak_ptr<Shader> shader, uint8_t flags, bool toPicker)
	{
		camera.setUniformPos("viewPosition", shader);
		camera.setUniformProj("projection", shader);
		camera.setUniformView("view", shader);

		auto lock = shader.lock();

		for (int i = 0; i < m_objects.size(); ++i)
		{
			if (toPicker && lock)
			{
				lock->setInt("index", i);
			}
			m_objects[i].m_object.draw(flags);
		}
	}

public:

	TreeItemPicker() = default;

	void initialize(unsigned int width, unsigned int height,
		std::shared_ptr<GpuMesh> mesh,
		std::shared_ptr<Shader> shader,
		std::shared_ptr<Shader> pickerShader,
		const std::vector<glm::mat4>& initialTransforms,
		const glm::vec3& halfBounds,
		float mass)
	{
		if (m_initialized) return;

		m_mesh = std::move(mesh);
		m_shader = shader;
		m_pickerShader = std::move(pickerShader);
		m_mass = mass;
		m_halfBounds = halfBounds;

		if (!m_mesh->isInitialized()) m_mesh->initialize();
		m_pickingTexture.initialize(width, height);

		for (const auto& t : initialTransforms)
			m_objects.push_back(addBox(t, m_halfBounds, m_mass));

		m_initialized = true;
	}

	void draw(const Camera& camera)
	{
		draw(camera, m_shader, 0, false);
	}

	void onClick(const unsigned int& x, const unsigned int& y, const  Camera& camera)
	{
		m_pickingTexture.enableWriting();
		draw(camera, m_pickerShader, static_cast<uint8_t>(RenderFlag::DontSendMat) | static_cast<uint8_t>(RenderFlag::DontSendMaps), true);
		m_pickingTexture.disableWriting();

		auto p = m_pickingTexture.readPixel(x, y);

		if (p.objectID > 0 && p.objectID <= m_objects.size())
		{
			freeObject(p.objectID - 1);
		}
	}

	void runSim(float deltaTime)
	{
		m_physics.runSimulation(deltaTime);
	}

};