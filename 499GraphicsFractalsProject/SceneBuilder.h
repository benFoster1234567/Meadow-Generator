#pragma once
#include <Renderer.h>
#include "TreeController.h"
#include "Plant.h"

struct FlowerConfig
{
	Material stemMat, flowerMat, budMat;
	Mesh petals, stem, bud, leaf1, leaf2, leaf3;
};

struct SceneBuilderConfig
{
	Material ground, bark, leaves;
	Mesh treeleaf;
	Mesh grass;
	std::pair<std::string_view, std::string_view> shader;
	std::pair<std::string_view, std::string_view> plantShader;
	std::pair<std::string_view, std::string_view> instancedShader;
	std::pair<std::string_view, std::string_view> leafShader;
	Light light;
	FlowerConfig flowerSettings;
};

struct SceneBuilderResult
{
	std::unique_ptr<TreeManager> trees;
	std::unique_ptr<Renderer> renderer;
};

//facade for easier setup
class SceneBuilder
{
public:


	SceneBuilder(SceneBuilderConfig sceneBuilder)
		:m_config(sceneBuilder)
	{
	}

	SceneBuilderResult BuildScene();


private:
	SceneBuilderConfig m_config;
	std::unique_ptr<TreeManager> m_treeManager{};
	std::unique_ptr<Plant> m_flower;
	std::unique_ptr<Plant> m_grass;
	bool m_built{ false };

	void setupLight(Renderer* renderer) const
	{
		renderer->setLight(m_config.light);
	}

	void setupPlants(Renderer* renderer, RenderPassID plantPass);
	
	void setupGround(Renderer* renderer, RenderPassID groundPass);

	void setupTrees(Renderer& renderer, RenderPassID treePass, RenderPassID leafPass);

	void initialize(Renderer* renderer)
	{
		m_flower->initialize();
		m_grass->initialize();
		renderer->initialize();
		
	}
};