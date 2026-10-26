#pragma once

#include "Shader.h"
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/gtc/quaternion.hpp>

class Camera
{
	float m_width;
	float m_height;

	glm::vec3 m_eye{ 1,1,0 };
	glm::vec3 m_target{ 0,0,0 };
	glm::vec3 m_up{ 0,1,0 };

	glm::mat4 m_projectionMat{};
	glm::mat4 m_viewMat{};

	float m_fov{ 100 };
public:
	Camera(float w, float h)
		: m_width(w), m_height(h)
	{
		reComputeView();
	}

	void reComputeView()
	{
		m_viewMat = glm::lookAt(m_eye, m_target, m_up);
	}

	void changeView(const glm::vec3& eye, const glm::vec3& target, const glm::vec3& up)
	{
		m_eye = eye;
		m_target = target;
		m_up = up;
		reComputeView();
	}

	void changeViewDistance(const float& distance)
	{
		using namespace glm;

		if (m_eye == m_target) return;

		vec3 offset = distance * normalize(m_eye - m_target);

		m_eye = m_target + offset;
	}

	void changeFov(const float& fov)
	{
		m_fov = fov;
		m_projectionMat = glm::perspective(glm::radians(60.0f), (GLfloat)m_width / m_height, 1.0f, m_fov);
	}

	void setViewAngleAroundTarget(const float& pitch, const float& yaw)
	{
		using namespace glm;
		float viewDistance = glm::length(m_eye - m_target);

		quat qPitch = angleAxis(-pitch, vec3(1.f, 0.f, 0.f)); // Rotate around X-axis
		quat qYaw = angleAxis(yaw, vec3(0.f, 1.f, 0.f)); // Rotate around Y-axis

		quat orientation = qYaw * qPitch;
		mat4 rot = mat4_cast(orientation);

		vec3 localOffset = vec3(0.f, 0.f, viewDistance);
		vec3 rotatedOffset = vec3(rot * vec4(localOffset, 1.f));

		m_eye = m_target + rotatedOffset;
	}

	void onWindowResize(const float& width, const float& height)
	{
		m_width = width;
		m_height = height;
		m_projectionMat = glm::perspective(glm::radians(m_fov), (GLfloat)width / height, .5f, 100.f);
	}

	void setUniformProj(std::string_view name, std::shared_ptr<Shader> shader) const
	{
		shader->setMat4(name.data(), m_projectionMat);
	}

	void setUniformPos(std::string_view name, std::weak_ptr<Shader> shader) const
	{
		if (auto lock = shader.lock())
		{
			lock->setVec3(name.data(), m_eye);
		}
	}

	void setUniformView(std::string_view name, std::weak_ptr<Shader> shader) const
	{
		if (auto lock = shader.lock())
		{
			lock->setMat4(name.data(), m_viewMat);
		}
	}
};