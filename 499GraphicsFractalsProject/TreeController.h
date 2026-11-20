#pragma once
#include "Mesh.h"
#include "MeshLoaderInstanced.h"
#include "TreeGenerator.h"
//#include "RenderedObject.h"
#include "Renderer.h"

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

	Renderer& m_renderer;
	ShaderID m_treeShader{};
	ShaderID m_leafShader{};
	MeshLoaderID m_leafID{};
	RenderableID m_leafRendererID{};

	std::vector<glm::mat4> m_leafInstances{};

	Material m_treeMat;
	Material m_leafMat;

	glm::mat4 worldTransform{ 1.0f };
	glm::mat4 localTransform{ 1.0f };
	
	Noise4D m_noise{};
	float time{ 0.f };

	std::vector<RenderableID> m_renderables{};

	//TODO: get leaves migrated -- 
public:
	TreeManager
	(
		Renderer& renderer
		, const Mesh& leafMesh
		, const Material& treeMat
		, const Material& leafMat
		, ShaderID treeShader
		, ShaderID leafShader
	)
		:
		m_renderer(renderer)
		, m_treeShader(treeShader)
		, m_leafShader(leafShader)
		, m_leafID(renderer.loadMeshInstanced(leafMesh))
		, m_treeMat(treeMat)
		, m_leafMat(leafMat)
	{
	}

	void setupTreeInstances()
	{
		const size_t maxLeafInstances = 8000;
		constexpr float textureHeight = 0.5f;

		//TODO: fix this for the new architecture
		size_t projectedLeafCount = m_leafInstances.size() * m_treeInstanceSettings.count;

		size_t leafStride = 1;
		if (maxLeafInstances < projectedLeafCount)
		{
			leafStride = projectedLeafCount / maxLeafInstances + 1;
		}

		std::vector<glm::mat4> leafInstancesNew{};

		TreeGenerator t{ 1,8 };

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

			MeshLoaderID mesh = m_renderer.loadMesh(t.getMesh());

			Renderable renderable
			{ 
				.mesh = mesh
				, .isInstanced = false
				, .hasRigidbody = false
				, .textureScale = 0.5f
				, .transform = Transform(treeTransform)
				, .material = m_treeMat 
			};

			RenderableID treeID = m_renderer.addRenderable(std::move(renderable));
			m_renderables.push_back(treeID);

			auto leaves = t.getLeafInstances();
			for (int leaf{ 0 }; leaf < leaves.size(); leaf += leafStride)
			{
				glm::mat4 leafTransform = m_renderer.getTransform(treeID).matrix() * leaves[leaf];
				leafInstancesNew.push_back(leafTransform);
			}
		}

		m_leafInstances = leafInstancesNew;
		m_renderer.changeInstances(m_leafID, leafInstancesNew);

		m_leafRendererID = m_renderer.addRenderable(
			{ 
				.mesh = m_leafID,
				.isInstanced = true,
				.hasRigidbody = false,
				.transform = Transform(glm::mat4(1))
			}
		);
	}

	void refreshTrees()
	{
		constexpr float textureHeight = 0.5f;
		TreeGenerator t{ 1,8 };
		std::vector<glm::mat4> newLeafInstances{};

		const size_t maxLeafInstances = 8000;
		size_t projectedLeafCount = t.computeLeafCountProjected(m_treeMeshSettings.levels, m_treeMeshSettings.n) * m_treeInstanceSettings.count;

		size_t leafStride = 1;
		if (maxLeafInstances < projectedLeafCount)
		{
			leafStride = projectedLeafCount / maxLeafInstances + 1;
		}

		m_leafInstances = t.getLeafInstances();
		for (auto& tree : m_renderables)
		{
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

			m_renderer.changeMesh(m_renderer.getRenderableMesh(tree), t.getMesh());

			auto leaves = t.getLeafInstances();
			for (int leaf{ 0 }; leaf < leaves.size(); leaf += leafStride)
			{
				glm::mat4 leafTransform = m_renderer.getTransform(tree).matrix() * leaves[leaf];
				newLeafInstances.push_back(leafTransform);
			}
		}

		m_leafInstances = newLeafInstances;
		m_renderer.changeInstances(m_leafID, newLeafInstances);
	}

	void refreshTrees(const glm::mat4& local, int levels, int n = 10, float deltaX = 30.5f, float deltaZ = 10.5f, float stride = 1.5f, float jitter = 5.f)
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

		refreshTrees();
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

	std::vector<glm::mat4> getAppleOrigins()
	{
		const int maxApples = 150;

		std::vector<glm::mat4> appleSeats{};
		const int appleStride = m_leafInstances.size() / maxApples + 1;

		for (int i{}; i < m_leafInstances.size(); i += appleStride)
		{
			appleSeats.push_back(m_leafInstances[i]);
		}

		return appleSeats;
	}


};