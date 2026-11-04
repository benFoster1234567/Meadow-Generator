#pragma once
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/gtc/quaternion.hpp>

class Transform
{
	glm::vec3 m_position{};
	glm::quat m_rotation{ 1.f, 0.f, 0.f, 0.f };
	glm::vec3 m_scale{ 1.f };

	glm::mat4 m_matrix{ 1 };
	bool m_dirty{ true };

public:

	void setPosition(const glm::vec3& position)
	{
		m_position = position;
		m_dirty = true;
	}

	void setRotation(const glm::vec3& eulerAngles)
	{
		m_rotation = glm::quat(eulerAngles);
		m_dirty = true;
	}

	void setRotation(const glm::quat& rot)
	{
		m_rotation = rot;
		m_dirty = true;
	}

	void setScale(const glm::vec3& scale)
	{
		m_scale = scale;
		m_dirty = true;
	}

	const glm::vec3& getPosition() const { return m_position; }
	const glm::quat& getRotation() const { return m_rotation; }
	const glm::vec3& getScale()    const { return m_scale; }
	glm::vec3 getEulerAngles() const { return glm::eulerAngles(m_rotation); }

	const glm::mat4& matrix()
	{
		if (m_dirty)
		{
			m_matrix = glm::translate(glm::mat4(1), m_position)
				* glm::mat4_cast(m_rotation)
				* glm::scale(glm::mat4(1), m_scale);
			m_dirty = false;
		}

		return m_matrix;
	}
};