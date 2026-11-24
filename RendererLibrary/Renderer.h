#pragma once
#include "Material.h"
#include "SparseSet.h"
#include "MeshLoader.h"
#include "Transform.h"

#include <array>
#include <string_view>
#include "Camera.h"
#include "MeshLoaderInstanced.h"
#include <deque>

#include "RendererIDs.h"
#include "Renderable.h"
#include "AnimatedNoise.h"


/*
 *
 * What objects do we need to render?
 * grass, flowers, trees, ground, leaves.
 *
 * What changes meshes/glbuffersubdata? trees, leaves.
 *
 * entity -> flags? could be
 * 	-> instanced
 * 	-> uniforms
 *
 * get entities we change,
 *  - changeInstances
 *  - changeMesh
 *  - upload changes
 *
 * stage shader
 * - set camera uniforms
 * - get all mesh loaders that use this shader
 * - set the subroutines
 * - for each mesh loader in here...
 * 	- get corresponding entities
 * 		- set uniforms
 * 			- materials, transform, etc..
 * 			- draw each entity
 *
 * How will we change the instances for only instanced entities/render passes?
 *
 * sparse sets of entity-owned components?
 *
 * SparseSet<InstanceData> instanceData ---- entityID -> data
 * SparseSet<Transform> transforms ----- entityID -> transform
 *
 * arrays of shared components:
 * Materials, Meshes, GpuMeshes
 *
 * Future requirements will include: shadows, shadow optimizations
 */

#define BIT(x) (1U << (x))

enum class RendererState
{
	Configure,
	Draw
};

enum class RenderPassFlags : uint8_t
{
	None = 0,
	SendAnimatedNoise = BIT(0),
	SendMaps = BIT(1),
	SendMaterials = BIT(2),
	SendTextureScale = BIT(3)
};

constexpr RenderPassFlags operator&(RenderPassFlags a, RenderPassFlags b) noexcept {
	return static_cast<RenderPassFlags>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

constexpr RenderPassFlags operator|(RenderPassFlags a, RenderPassFlags b) noexcept {
	return static_cast<RenderPassFlags>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

struct RenderPass
{
	ShaderID shader{};
	RenderPassFlags flags{RenderPassFlags::None};
	std::vector<RenderableID> renderables{};
};

class Renderer
{
public:
	static constexpr size_t MaxSharedComponentCount = 100;

	ShaderID addShader(std::string_view vertexFile, std::string_view fragFile);
	ShaderID addShader(std::pair<std::string_view, std::string_view> filenames)
	{
		return addShader(filenames.first, filenames.second);
	}

	RenderableID addRenderable(Renderable&& renderable);
	void setTransform(RenderableID id, const Transform& transform);
	Transform& getTransform(RenderableID id);

	void attachRenderableToRenderPass(RenderPassID pass, RenderableID renderable);
	MeshLoaderID getRenderableMesh(RenderableID renderable) { return m_renderables[renderable].mesh; }

	RenderPassID addRenderPass(ShaderID shader, RenderPassFlags flags = RenderPassFlags::SendMaterials)
	{
		RenderPassID id = m_renderPasses.size();
		RenderPass renderPass{ .shader = shader, .flags = flags};
		m_renderPasses.push_back(renderPass);

		return id;
	}

	MeshLoaderID loadMesh(const Mesh& mesh);
	MeshLoaderID loadMeshInstanced(const Mesh& mesh);

	void changeMesh(MeshLoaderID id, Mesh mesh);
	void changeInstances(MeshLoaderID id, const std::vector<glm::mat4>& instances);

	void initialize();

	void draw(Camera& camera);
	void updateNoise(float deltaTime)
	{
		m_noise.updateNoiseTexture(deltaTime);
	}
	void setLight(const Light& light)
	{
		m_light = light;
	}

private:
	SparseSet<MeshLoader> m_meshLoaders{};
	SparseSet<MeshLoaderInstanced> m_meshLoadersInstanced{};

	Light m_light{};

	std::vector<Shader> m_shaders{};

	RendererState m_state = RendererState::Configure;
	AnimatedNoise m_noise{};
	GLuint m_noiseMap{};
	MeshLoaderID m_nextMesh = 1;
	std::vector<Renderable> m_renderables{};
	std::vector<RenderPass> m_renderPasses{};

	void sendLightToShaders(const Light& light)
	{
		for (auto& shader : m_shaders)
		{
			m_light = light;
			shader.setLight(light);
		}
	}
};
