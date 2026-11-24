#include "Renderer.h"

ShaderID Renderer::addShader(std::string_view vertexFile, std::string_view fragFile)
{
	ShaderID id = m_shaders.size();
	m_shaders.emplace_back();

	if (!m_shaders.back().loadShader(vertexFile, fragFile))
	{
		throw std::runtime_error("addShader failed for "
			+ std::string(vertexFile) + " and "
			+ std::string(fragFile));
	}

	return id;
}

RenderableID Renderer::addRenderable(Renderable&& renderable)
{
	RenderableID id = m_renderables.size();
	m_renderables.emplace_back(std::move(renderable));
	m_renderables.back().id = id;
	return id;
}

void Renderer::setTransform(RenderableID id, const Transform& transform)
{
	m_renderables.at(id).transform = transform;
}

Transform& Renderer::getTransform(RenderableID id)
{
	return m_renderables.at(id).transform;
}

void Renderer::attachRenderableToRenderPass(RenderPassID pass, RenderableID renderable)
{
	m_renderPasses[pass].renderables.push_back(renderable);
}

MeshLoaderID Renderer::loadMesh(const Mesh& mesh)
{
	MeshLoaderID id = m_nextMesh;
	m_nextMesh++;
	m_meshLoaders.insert(id, MeshLoader(mesh));
	return id;
}

MeshLoaderID Renderer::loadMeshInstanced(const Mesh& mesh)
{
	MeshLoaderID id = m_nextMesh;
	m_nextMesh++;
	m_meshLoadersInstanced.insert(id, MeshLoaderInstanced(mesh));
	return id;
}

void Renderer::changeMesh(MeshLoaderID id, Mesh mesh)
{
	if (m_meshLoaders.contains(id))
	{
		m_meshLoaders.get(id).changeMesh(std::move(mesh));
		return;
	}

	if (m_meshLoadersInstanced.contains(id))
	{
		m_meshLoadersInstanced.get(id).changeMesh(std::move(mesh));
		return;
	}

	throw std::out_of_range("Mesh loader ID does not exist");
}

void Renderer::changeInstances(MeshLoaderID id, const std::vector<glm::mat4>& instances)
{
	if (!m_meshLoadersInstanced.contains(id))
	{
		throw std::out_of_range("Instanced mesh loader ID does not exist");
	}

	m_meshLoadersInstanced.get(id).changeInstances(instances);
}

void Renderer::initialize()
{

	if (m_state >= RendererState::Draw)
	{
		throw std::runtime_error("Renderer has already been initialized");
	}

	m_noiseMap = m_noise.generateNoiseTexture(32, 12.f);

	for (auto& s : m_shaders)
	{
		s.useShader();
		s.setLight(m_light);
	}

	for (auto& m : m_meshLoaders)
	{
		m.initialize();
	}

	for (auto& m : m_meshLoadersInstanced)
	{
		m.initialize();
	}
}

void Renderer::draw(Camera& camera)
{
	for (size_t rp = 0; rp < m_renderPasses.size(); ++rp)
	{
		const RenderPass& renderPass = m_renderPasses[rp];
		auto& shader = m_shaders[renderPass.shader];
		shader.useShader();

		shader.setMat4("view", camera.view());
		shader.setMat4("projection", camera.projection());
		shader.setVec3("viewPosition", camera.eye());

		bool sendMaterials = (RenderPassFlags::SendMaterials & renderPass.flags) != RenderPassFlags::None;
		bool sendMaps = (RenderPassFlags::SendMaps & renderPass.flags) != RenderPassFlags::None;
		bool sendNoise = (RenderPassFlags::SendAnimatedNoise & renderPass.flags) != RenderPassFlags::None;

		for (size_t rid = 0; rid < renderPass.renderables.size(); ++rid)
		{
			Renderable& renderable = m_renderables[renderPass.renderables[rid]];

			renderable.setupDraw();

			shader.setMat4("model", renderable.transform.matrix());

			if (sendNoise)
			{
				shader.bindTexture("noiseTexture", 2, m_noiseMap);
			}

			if (sendMaterials)
			{
				shader.setMaterial(renderable.material, sendMaps);
				shader.setMat3("texture", renderable.textureScale);
			}

			if (renderable.isInstanced)
			{
				m_meshLoadersInstanced.get(renderable.mesh).draw();
			}

			else
			{
				m_meshLoaders.get(renderable.mesh).draw();
			}
		}
	}
}
