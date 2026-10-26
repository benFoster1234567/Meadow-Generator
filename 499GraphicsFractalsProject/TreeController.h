#pragma once
#include "Mesh.h"
#include "OpenGL.h"
#include "InstancedMesh.h"
#include "TreeGenerator.h"


class TreeManager
{
	InstancedMesh leaves;
	InstancedMesh tree;

	Material treeMat;
	Material leafMat;

	glm::mat4 worldTransform{ 1.0f };
	glm::mat4 localTransform{ 1.0f };
	std::weak_ptr<Shader> m_treeShader;
	std::weak_ptr<Shader> m_leafShader;

	Noise4D m_noise{};
	float time{ 0.f };
public:
	TreeManager
	(
		const Mesh& leafMesh
		, const Material& treeMat
		, const Material& leafMat
		, std::weak_ptr<Shader> treeShader
		, std::weak_ptr<Shader> leafShader
	)
		:
		leaves(leafMesh)
		, treeMat(treeMat)
		, leafMat(leafMat)
		, m_treeShader(treeShader)
		, m_leafShader(leafShader)
	{
	}

	void setShaders(std::weak_ptr<Shader> treeShader, std::weak_ptr<Shader> leafShader)
	{
		m_treeShader = treeShader;
		m_leafShader = leafShader;
	}

	void setTransform(const glm::mat4& t)
	{
		worldTransform = t;
	}

	void setUpTree(glm::mat4 local, int levels, int n = 10, float deltaX = 30.5f, float stride = 1.5f)
	{
		localTransform = local;
		tree.addInstance(worldTransform);
		TreeGenerator t{ 4,16 };
		t.generateExampleTree(local, levels, 10, { 1,1,1 }, deltaX, 10.5f, stride, 5.0f, 30);
		tree.changeMesh(t.getMesh());
		leaves.changeInstances(t.getLeafInstances());
	}

	void initialize()
	{
		if (auto lock = m_treeShader.lock())
		{
			lock->useShader();
			tree.initialize();
		}

		if (auto lock = m_leafShader.lock())
		{
			leaves.initialize();
		}
	}

	void changeLevels(int levels)
	{
		setUpTree(localTransform, levels);
	}

	void changeLeafStride(int stride)
	{
		leaves.changeInstanceStride(stride);
	}

	void tick(float deltaTime)
	{
		time += deltaTime;
		glm::vec3 noisePos = glm::vec3(m_noise.compute({ 1,0,0,time * 0.001f })
			, m_noise.compute({ 0,1,0,time * 0.1f })
			, m_noise.compute({ 0,0,1,time }));

		float theta = noisePos.z - 0.5f;
		float thetaX = noisePos.x - 0.5f;

		std::vector<VertexData> leafCopy = leaves.m_mesh.m_data;

		for (auto& value : leafCopy)
		{
			value.point = glm::vec3(glm::rotate(glm::mat4(1), (theta * 0.1f), glm::vec3(0, 0, 1)) *
				glm::rotate(glm::mat4(1), thetaX, glm::vec3(1, 0, 0)) * glm::vec4(value.point, 1.0f));
		}

		// superstitious
		leaves.m_mesh.m_data = leafCopy;
		leaves.changeMesh(leaves.m_mesh);
	}


	void draw()
	{
		if (auto lock = m_treeShader.lock())
		{
			lock->setMat4("model", worldTransform);
			lock->setMaterial(treeMat);
			tree.draw();
		}

		if (auto lock = m_leafShader.lock())
		{
			lock->setMat4("model", worldTransform);
			lock->setMaterial(leafMat);
			leaves.draw();
		}
	}


};