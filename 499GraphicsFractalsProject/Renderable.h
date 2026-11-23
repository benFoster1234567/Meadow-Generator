#pragma once

#include "RendererIDs.h"
#include "Transform.h"
#include "Material.h"

struct Renderable
{
	RenderableID id{};
	MeshLoaderID mesh{};

	bool isInstanced = false;
	bool hasRigidbody = false;

	glm::mat3 textureScale{ 1 };
	Transform transform{};
	Material material{};

	RigidbodyID rigidbody{};

	std::function<void()> setupDraw = []() {};
};