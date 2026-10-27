#pragma once
#include "Mesh.h"
#include "InstancedMesh.h"
#include "TreeGenerator.h"


class TreeManager
{
	struct
	{
		size_t count{ 1 };
		float posJitter{ 0.f };
		float rotYJitter{ 0.f };
		float spacing{ 1.f };
		float yOffset{ 0.0f };
	}m_treeInstanceSettings{};

	InstancedMesh leaves;
	InstancedMesh tree;

	std::vector<glm::mat4> leafInstancesCached{};

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


	void setCount(size_t count)
	{
		m_treeInstanceSettings.count = count;
	}

	void setPosJitter(float jitter)
	{
		m_treeInstanceSettings.posJitter = jitter;
	}

	void setSpacing(float spacing)
	{
		m_treeInstanceSettings.spacing = spacing;
	}

	void setRotJitter(float jitter)
	{
		m_treeInstanceSettings.rotYJitter = jitter;
	}

	//todo: keep this out of instance matrix. Just use regular transform matrix for it...
	void setyOffset(float h)
	{
		m_treeInstanceSettings.yOffset = h;
	}

	void reloadTreeInstances()
	{
		const size_t maxLeafInstances = 3000;
		size_t projectedLeafCount = leafInstancesCached.size() * m_treeInstanceSettings.count;

		size_t leafStride = 1;
		if (maxLeafInstances < projectedLeafCount)
		{
			leafStride = projectedLeafCount / maxLeafInstances + 1;
		}

		std::vector<glm::mat4> treeInstances{};
		std::vector<glm::mat4> leafInstancesNew{};

		int sideLen = static_cast<int>(glm::sqrt(static_cast<double>(m_treeInstanceSettings.count) + 0.99999999));
		int offset = sideLen / 2;
		float phi = std::clamp<float>(1.0f - m_treeInstanceSettings.rotYJitter, 0.002f, 1.0f);
		float theta;
		theta = std::clamp(1 / phi, 1.0f, 360.f);
		for (int i = 0; i < m_treeInstanceSettings.count; ++i)
		{
			theta = std::fmod(theta * theta, 360.f);
			glm::mat4 rotTree = glm::mat4_cast(glm::angleAxis(glm::radians(theta), glm::vec3(0, 1, 0)));

			float px = m_treeInstanceSettings.spacing * static_cast<float>(i % sideLen - offset);
			float pz = m_treeInstanceSettings.spacing * static_cast<float>(i / sideLen - offset);
			glm::mat4 translateTree = glm::translate(glm::mat4(1), glm::vec3{ px, m_treeInstanceSettings.yOffset, pz });

			glm::mat4 treeTransform = translateTree * rotTree;

			treeInstances.push_back(treeTransform);

			for (int leaf{ 0 }; leaf < leafInstancesCached.size(); leaf += leafStride)
			{
				glm::mat4 leafTransform = treeTransform * leafInstancesCached[leaf];
				leafInstancesNew.push_back(leafTransform);
			}
		}

		tree.changeInstances(treeInstances);
		leaves.changeInstances(leafInstancesNew);

		std::cout << "leaf instance count: " << leaves.instanceCount() << "\n";


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

	void setUpTree(const glm::mat4& local, int levels, int n = 10, float deltaX = 30.5f, float stride = 1.5f)
	{
		localTransform = local;
		tree.addInstance(worldTransform);
		TreeGenerator t{ 1,8 };
		t.generateExampleTree(local, levels, 10, { 1,1,1 }, deltaX, 10.5f, stride, 5.0f, 30);
		//t.generateTreeWithNoise(local, 10, 5, 1);
		leafInstancesCached = t.getLeafInstances();
		tree.changeMesh(t.getMesh());

		if (tree.instanceCount() > 1)
		{
			reloadTreeInstances();
		}
		else
		{
			tree.changeMesh(t.getMesh());
			leaves.changeInstances(leafInstancesCached);
		}
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

	void changeLevels(const int& levels)
	{
		setUpTree(localTransform, levels);
	}

	void changeLeafStride(const int& stride)
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
			lock->useShader();
			lock->setMat4("model", worldTransform);
			lock->setMat3("texture", glm::mat3(10));
			lock->setMaterial(treeMat);
			tree.draw();
		}

		if (auto lock = m_leafShader.lock())
		{
			lock->useShader();
			lock->setMat4("model", worldTransform);
			lock->setMat3("texture", glm::mat3(10));
			lock->setMaterial(leafMat);
			leaves.draw();
		}
	}


};