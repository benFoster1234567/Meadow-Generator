#include "SceneBuilder.h"

SceneBuilderResult SceneBuilder::BuildScene()
{
	if (m_built)
	{
		throw std::runtime_error("scene can't be build twice");
	}
	else
	{
		m_built = true;
	}

	auto r = std::make_unique<Renderer>();
	//shader config...

	auto plantShader = r->addShader(m_config.plantShader);
	auto groundShader = r->addShader(m_config.shader);
	auto leafShader = r->addShader(m_config.leafShader);
	auto plantPass = r->addRenderPass(plantShader, RenderPassFlags::SendMaterials | RenderPassFlags::SendMaps | RenderPassFlags::SendAnimatedNoise);
	auto groundPass = r->addRenderPass(groundShader, RenderPassFlags::SendMaterials | RenderPassFlags::SendMaps | RenderPassFlags::SendTextureScale);
	auto treePass = r->addRenderPass(groundShader, RenderPassFlags::SendMaterials | RenderPassFlags::SendMaps | RenderPassFlags::SendTextureScale);
	auto leafPass = r->addRenderPass(leafShader, RenderPassFlags::SendMaterials | RenderPassFlags::SendAnimatedNoise);

	r->attachShadowSetup(groundPass);

	setupLight(r.get());
	setupPlants(r.get(), plantPass);
	setupGround(r.get(), groundPass);
	setupTrees(*r, treePass, leafPass);
	initialize(r.get());


	return SceneBuilderResult{ .trees = std::move(m_treeManager), .renderer = std::move(r) };
}

void SceneBuilder::setupPlants(Renderer* renderer, RenderPassID plantPass)
{
	glm::mat4 flowerTransform{ 1 };

	m_flower = std::make_unique<Plant>(*renderer);
	m_grass = std::make_unique<Plant>(*renderer);

	FlowerConfig& flowerSettings = m_config.flowerSettings;
	Mesh::unitize({ flowerSettings.petals, flowerSettings.bud,flowerSettings.leaf1,flowerSettings.leaf2,flowerSettings.leaf3,flowerSettings.stem });
	Mesh::unitize({ m_config.grass });

	auto stem = renderer->loadMeshInstanced(m_config.flowerSettings.stem);
	auto petals = renderer->loadMeshInstanced(m_config.flowerSettings.petals);
	auto bud = renderer->loadMeshInstanced(m_config.flowerSettings.bud);
	auto leaf1 = renderer->loadMeshInstanced(m_config.flowerSettings.leaf1);
	auto leaf2 = renderer->loadMeshInstanced(m_config.flowerSettings.leaf2);
	auto leaf3 = renderer->loadMeshInstanced(m_config.flowerSettings.leaf3);
	auto grass = renderer->loadMeshInstanced(m_config.grass);

	ModelComponent petalComponent{ petals, flowerTransform, flowerSettings.flowerMat, plantPass };
	petalComponent.colors.push_back(glm::vec3(1.f, 1.f, 0.f));
	petalComponent.colors.push_back(glm::vec3(0.f, 0.5f, 1.f));
	petalComponent.colors.push_back(glm::vec3(1.f, 0.0f, 0.5f));

	m_flower->addComponent(petalComponent);
	m_flower->addComponent(stem, flowerTransform, flowerSettings.stemMat, plantPass);
	m_flower->addComponent(bud, flowerTransform, flowerSettings.budMat, plantPass);

	m_flower->arrangePlantsUsingFractal(5, 5, 18);

	m_grass->arrangePlantsUsingFractal(7, 6, 18);
	m_grass->addComponent(grass, glm::translate(glm::mat4(1.0f), { 0,0,0 }) * glm::scale(glm::mat4(1.f), glm::vec3(0.5, .3, .5)), m_config.ground, plantPass);
	m_grass->setTransform(glm::scale(glm::mat4(1), glm::vec3(.5f, .5f, .5f)));

}

void SceneBuilder::setupGround(Renderer* renderer, RenderPassID groundPass)
{
	MeshLoaderID planeID = renderer->loadMesh(Mesh(Plane{}, 10.f));
	auto groundID = renderer->addRenderable({ .mesh = planeID, .isInstanced = false, .hasRigidbody = false, .textureScale = glm::mat3(10.f), .transform = Transform(glm::scale(glm::mat4(1), glm::vec3(5))), .material = m_config.ground });
	renderer->attachRenderableToRenderPass(groundPass, groundID);
}

void SceneBuilder::setupTrees(Renderer& renderer, RenderPassID treePass, RenderPassID leafPass)
{
	m_treeManager = std::make_unique<TreeManager>(renderer, m_config.treeleaf, m_config.bark, m_config.leaves, treePass, leafPass);
	m_treeManager->setCount(30);
	m_treeManager->setSpacing(10);
	m_treeManager->setPosJitter(10.f);
	m_treeManager->setRotJitter(0.9f);
	m_treeManager->setupTreeInstances();
	m_treeManager->refreshTrees(glm::translate(glm::mat4(1), glm::vec3(0, -0.01f, 0)) * glm::scale(glm::mat4(1), glm::vec3(0.18f, 0.18f, 0.18f)), 5, 11, 80.f, 1.05f);
}
