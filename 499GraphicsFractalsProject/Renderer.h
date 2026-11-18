#pragma once
#include "Material.h"
#include "SparseSet.h"
#include "MeshLoader.h"
#include "Transform.h"

#include <array>
#include "Camera.h"

/*
* 
What objects do we need to render?
grass, flowers, trees, ground, leaves.

What changes meshes/glbuffersubdata? trees, leaves.

entity -> flags? could be
	-> instanced
	-> uniforms

get entities we change, 
 - changeInstances
 - changeMesh
upload changes

stage shader
- set camera uniforms
- get all mesh loaders that use this shader
- set the subroutines
- for each mesh loader in here...
	- get corresponding entities
		- set uniforms
			- materials, transform, etc..
			- draw each entity

How will we change the instances for only instanced entities/render passes?

sparse sets of entity-owned components?

SparseSet<InstanceData> instanceData ---- entityID -> data
SparseSet<Transform> transforms ----- entityID -> transform

arrays of shared components: 
Materials, Meshes, GpuMeshes

Future requirements will include: shadows, shadow optimizations
*/

using Entity = size_t;
using ShaderID = size_t;
using ComponentType = size_t;
using MeshID = size_t;
using MeshLoaderID = size_t;
using MeshLoaderInstancedID = size_t;
using SubRoutineID = size_t;


enum class RendererState
{
	
};

// could probably split this up since it manages so many different things?
//leaving it like this for now though
class Renderer
{
public:
	static constexpr size_t MaxSharedComponentCount = 100;

	Entity createEntity();

	bool attachMesh(Entity entity, MeshLoaderID mesh);
	bool attachMeshInstanced(Entity entity, MeshLoaderInstancedID mesh);

	const std::vector<glm::mat4>& getMeshInstances(Entity entity, bool* success);
	bool changeMeshInstances(Entity entity, const std::vector<glm::mat4>& instanceData);

	void setMaterial(Entity entity);

	void setTransform(Entity entity, Transform transform);
	Transform& changeTransform(Entity);

	SubRoutineID configureShaderSubroutine(ShaderID shader);

	bool attachShader(Entity, ShaderID);

	ShaderID addShader(std::string_view vertexFile, std::string_view fragFile);

	MeshLoaderID loadMesh(const Mesh& mesh);
	MeshLoaderInstancedID loadMeshInstanced(const Mesh& mesh);

	void draw(Camera& camera);

private:
	//Shared components - not unique
	std::array<Material, MaxSharedComponentCount> m_materials{};
	std::array<MeshLoader, MaxSharedComponentCount> m_meshLoaders{};


};