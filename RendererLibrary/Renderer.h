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


enum class RendererState
{
	Configure,
	Draw
};

struct RenderPass
{
	ShaderID shader{};
	bool sendMatMaps{};
	bool sendMaterials{};
	std::vector<RenderableID> renderables{};
};

class Renderer
{
public:
	static constexpr size_t MaxSharedComponentCount = 100;

	ShaderID addShader(std::string_view vertexFile, std::string_view fragFile);

	RenderableID addRenderable(Renderable&& renderable);
	void setTransform(RenderableID id, const Transform& transform);
	Transform& getTransform(RenderableID id);

	void attachRenderableToShaderRenderPass(ShaderID shader, RenderableID renderable);
	MeshLoaderID getRenderableMesh(RenderableID renderable) { return m_renderables[renderable].mesh; }

	void configureRenderPass(ShaderID shader, bool sendMaps, bool sendMaterials)
	{
		auto& renderPass = m_renderPasses[shader];
		renderPass.sendMaterials = sendMaterials;
		renderPass.sendMatMaps = sendMaps;
	}



	MeshLoaderID loadMesh(const Mesh& mesh);
	MeshLoaderID loadMeshInstanced(const Mesh& mesh);

	void changeMesh(MeshLoaderID id, Mesh mesh);
	void changeInstances(MeshLoaderID id, const std::vector<glm::mat4>& instances);

	void initialize();

	void draw(Camera& camera);

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
