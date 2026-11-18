#pragma once

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
- get all gpumeshes that use this shader
- set the subroutines
- for each gpuMesh in here...
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

using ShaderID = size_t;
using ComponentType = size_t;

enum class RendererState
{
	

};

class Renderer
{

};