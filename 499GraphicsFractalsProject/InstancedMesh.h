#pragma once
#include "GL/glew.h"
#include "Mesh.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <vector>
#include <ranges>
#include "GpuMesh.h"

struct InstanceData
{
	glm::vec3 position{};
	glm::vec3 rotationEuler{};
	glm::vec3 scale{};
	glm::vec4 color{};
};
//TODO: Fix this
class InstancedMesh : public GpuMesh
{
private:
	std::vector<glm::mat4> m_instances{};

	void releaseInstanceBuffer()
	{
		if (m_ibo) glDeleteBuffers(1, &m_ibo);
		m_ibo = 0;
		m_capacity = 0;
		m_drawCount = 0;
	}

	static glm::mat4 packInstance(const InstanceData& instance)
	{
		glm::mat4 pos = glm::translate(glm::mat4(1), instance.position);
		glm::mat4 rot = glm::toMat4(glm::quat(instance.rotationEuler));
		glm::mat4 scale = glm::scale(glm::mat4(1), instance.scale);

		glm::mat4 transform = pos * rot * scale;

		transform[0].w = instance.color.r;
		transform[1].w = instance.color.g;
		transform[2].w = instance.color.b;
		transform[3].w = instance.color.a;

		return transform;
	}

	static std::vector<glm::mat4> packAll(const std::vector<InstanceData>& src)
	{
		std::vector<glm::mat4> result;
		result.reserve(src.size());
		for (const auto& i : src) result.push_back(packInstance(i));
		return result;
	}

	GLuint m_ibo{};
	size_t m_capacity = 0;
	size_t m_drawCount = 0;
public:
	InstancedMesh() = default;

	explicit InstancedMesh(Mesh mesh)
		: GpuMesh(std::move(mesh)) {
	}

	InstancedMesh(Mesh mesh, std::vector<glm::mat4> instances)
		: GpuMesh(std::move(mesh)), m_instances(std::move(instances)) {
	}

	InstancedMesh(Mesh mesh, const std::vector<InstanceData>& instances)
		: GpuMesh(std::move(mesh)), m_instances(packAll(instances)) {
	}


	InstancedMesh(const InstancedMesh&) = delete;
	InstancedMesh& operator=(const InstancedMesh&) = delete;

	InstancedMesh(InstancedMesh&& other) noexcept
		:
		GpuMesh(std::move(other)),
		m_ibo(std::exchange(other.m_ibo, 0)),
		m_instances(std::move(other.m_instances)) {
	}

	InstancedMesh& operator=(InstancedMesh&& other) noexcept
	{
		if (this != &other)
		{
			release();
			m_vao = std::exchange(other.m_vao, 0);
			m_vbo = std::exchange(other.m_vbo, 0);
			m_ebo = std::exchange(other.m_ebo, 0);
			m_ibo = std::exchange(other.m_ibo, 0);
			m_mesh = std::move(other.m_mesh);
			m_instances = std::move(other.m_instances);
		}
		return *this;
	}

	~InstancedMesh() override { releaseInstanceBuffer(); }

	void addInstance(const InstanceData& data)
	{
		m_instances.push_back(packInstance(data));
	}

	void addInstance(const glm::mat4& data)
	{
		m_instances.push_back(data);
	}

	void changeInstancesTemp(const std::vector<glm::mat4>& newInstances)
	{
		if (m_vao == 0)
		{
			return;
		}

		glBindVertexArray(m_vao);
		glBindBuffer(GL_ARRAY_BUFFER, m_ibo);
		glBufferData(GL_ARRAY_BUFFER, newInstances.size() * sizeof(glm::mat4), newInstances.data(), GL_DYNAMIC_DRAW);
		glBindVertexArray(0);
	}

	size_t instanceCount()
	{
		return m_instances.size();
	}

	void changeInstances(const std::vector<glm::mat4>& newInstances)
	{
		m_instances = newInstances;
		changeInstancesTemp(m_instances);
	}

	void initializeAdditionalAttributes() override
	{
		releaseInstanceBuffer();
		glGenBuffers(1, &m_ibo);
		glBindBuffer(GL_ARRAY_BUFFER, m_ibo);
		glBufferData(GL_ARRAY_BUFFER, m_instances.size() * sizeof(glm::mat4), m_instances.data(), GL_DYNAMIC_DRAW);

		for (GLuint i = 0; i < 4; ++i) // puts the whole matrix into the shaders as 4 vec4s
		{
			GLuint attribIndex = 3 + i;
			GLuint stride = sizeof(glm::mat4);
			GLuint size = 4;
			glVertexAttribPointer(attribIndex, size, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(glm::vec4) * i));
			glEnableVertexAttribArray(attribIndex);
			glVertexAttribDivisor(attribIndex, 1);
		}
	}

	const std::vector<glm::mat4>& getInstances()
	{
		return m_instances;
	}

	void changeInstanceStride(size_t stride)
	{
		stride += 1;

		std::vector<glm::mat4> newInst{};

		for (int i{}; i < m_instances.size(); ++i)
		{
			if (i % stride == 0)
			{
				newInst.push_back(m_instances[i]);
			}
		}

		changeInstancesTemp(newInst);
	}

	void draw() override
	{
		glBindVertexArray(m_vao);
		glDrawElementsInstanced(GL_TRIANGLES, m_mesh.m_indices.size(), GL_UNSIGNED_INT, (void*)0, m_instances.size());
		glBindVertexArray(0);
	}


};