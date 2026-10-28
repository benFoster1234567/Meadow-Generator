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

	struct
	{
		glm::mat4 local{ 1 };
		size_t levels{ 10 };
		size_t n{ 10 };
		float deltaX{ 30.5f };
		float deltaZ{ 10.5f };
		float girth{ 1.f };
		float stride{ 1.f };
		float jitter{ 1.f };
	}m_treeMeshSettings{};

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



	void reloadTreeInstances()
	{
		const size_t maxLeafInstances = 8000;
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



	void refreshTree()
	{
		constexpr float textureHeight = 0.5f;

		tree.addInstance(worldTransform);
		TreeGenerator t{ 1,8 };

		t.generateExampleTree(
			m_treeMeshSettings.local
			, m_treeMeshSettings.levels
			, m_treeMeshSettings.n
			, { 2,2,2 }
			, m_treeMeshSettings.deltaX
			, m_treeMeshSettings.deltaZ
			, m_treeMeshSettings.stride
			, m_treeMeshSettings.jitter
			, textureHeight);

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

	void refreshTree(const glm::mat4& local, int levels, int n = 10, float deltaX = 30.5f, float deltaZ = 10.5f, float stride = 1.5f, float jitter = 5.f)
	{
		m_treeMeshSettings =
		{
			.local = local,
			.levels = static_cast<size_t>(levels),
			.n = static_cast<size_t>(n),
			.deltaX = deltaX,
			.deltaZ = deltaX,
			.stride = stride,
			.jitter = jitter
		};

		refreshTree();
	}

	void setShaders(std::weak_ptr<Shader> treeShader, std::weak_ptr<Shader> leafShader)
	{
		m_treeShader = treeShader;
		m_leafShader = leafShader;
	}



	void setTransform(const glm::mat4& t) { worldTransform = t; }
	void setLocalTransform(const glm::mat4& t) { m_treeMeshSettings.local = t; }
	void setCount(size_t count) { m_treeInstanceSettings.count = count; }
	void setPosJitter(float jitter) { m_treeInstanceSettings.posJitter = jitter; }
	void setSpacing(float spacing) { m_treeInstanceSettings.spacing = spacing; }
	void setRotJitter(float jitter) { m_treeInstanceSettings.rotYJitter = jitter; }
	void setYOffset(float h) { m_treeInstanceSettings.yOffset = h; }
	void setDeltaX(const float& deltaX) { m_treeMeshSettings.deltaX = deltaX; }
	void setDeltaZ(const float& deltaZ) { m_treeMeshSettings.deltaZ = deltaZ; }
	void setGirth(const float& girth) { m_treeMeshSettings.girth = girth; }
	void setStride(const float& stride) { m_treeMeshSettings.stride = stride; }
	void setJitter(const float& jitter) { m_treeMeshSettings.jitter = jitter; }
	void setLevels(const size_t& levels) { m_treeMeshSettings.levels = levels; }
	void setN(const size_t& n) { m_treeMeshSettings.n = n; }

	size_t getLevels() const { return m_treeMeshSettings.levels; }
	size_t getN() const { return m_treeMeshSettings.n; }
	size_t getCount() const { return m_treeInstanceSettings.count; }
	float getDeltaX() const { return m_treeMeshSettings.deltaX; }
	float getDeltaZ() const { return m_treeMeshSettings.deltaZ; }
	float getGirth() const { return m_treeMeshSettings.girth; }
	float getStride() const { return m_treeMeshSettings.stride; }
	float getJitter() const { return m_treeMeshSettings.jitter; }
	float getPosJitter() const { return m_treeInstanceSettings.posJitter; }
	float getSpacing() const { return m_treeInstanceSettings.spacing; }
	float getRotJitter() const { return m_treeInstanceSettings.rotYJitter; }
	float getYOffset() const { return m_treeInstanceSettings.yOffset; }

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
			lock->setMat3("texture", glm::mat3(0.5f));
			lock->setMaterial(treeMat);
			tree.draw();
		}

		if (auto lock = m_leafShader.lock())
		{
			lock->useShader();
			lock->setMat4("model", worldTransform);
			lock->setMat3("texture", glm::mat3(.5f));
			lock->setMaterial(leafMat);
			leaves.draw();
		}
	}


};