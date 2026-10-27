#define ENSURE_GPU_RENDERED
#define LOG_DEBUG_INFO

#ifdef _WIN32
#ifdef ENSURE_GPU_RENDERED
#include <windows.h>

extern "C" {
	__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;// Optimus: force switch to discrete GPU
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;//AMD
}
#endif
#endif

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "GLDebugMessageHandler.h"

#include "Window.h"
#include "Mesh.h"

#include "Ui.h"
#include "Plant.h"

#include "TreeController.h"
#include "Camera.h"
#include "ShaderManager.h"

#include <IL/il.h>
#include <IL/ilu.h>
#include <IL/ilut.h>

#include <thread>

Material ground, bark, leaves;
Mesh plane(Plane{}, 10.f);

Camera camera{ 1000,700 };
ShaderManager shaderManager{};

ShaderManager::ShaderIndex leavesShader, barkShader, plantShader, baseShader;

Plant plant{};
Plant flower{};
Plant groundPlane{};


float cameraSpeed{ 0.f };

InstancedMesh leafMesh{ Mesh{Plane{}, 0.5f} };

std::unique_ptr<TreeManager> trees{ nullptr };
bool loading = false;

/*

TODO:
- Fix tree texturing
- optimize memory and cpu usage
- Multiple trees *
- Fix sizes
- add apple picking

BackBurner:
- make tree generation thread safe
- do heavy stuff on a background thread

*/
std::atomic<bool> isInitialized{ false };
int levels = 5;

GLuint loadTexture(const char* fileName);
void initialize();
void setupScene();
void generatePlants();
void display(Window& window, float deltaTime);

int main(int argc, char** argv)
{
	if (!glfwInit()) return -1;
	Window window(1000, 700);
	window.makeContextCurrent();

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		DBG_LOG(DebugMsgType::GLError, "GLEW Init failed!\n");
	}

	DebugHandler::setUpOpenGL();
	DebugHandler::instance().setTypeRecord(DebugMsgType::GLStatus, false);
	DebugHandler::instance().setTypeRecord(DebugMsgType::Default, false);

	ilInit();

	initialize();

	std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
	std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;

	int number = 0;
	ImGuiUi ui;
	ui.setup(window);

	int leafStride = 0;

	auto* text = static_cast<Text*>(ui.addElement(std::make_unique<Text>("")));
	auto* slider = ui.addElement(std::make_unique<SliderFloat>(&cameraSpeed, "Camera Orbit Speed", 0.f, 20.f));
	auto* sliderLevels = ui.addElement(std::make_unique<SliderInt>(&levels, "Level", 0, 15));
	auto* sliderLeafStride = ui.addElement(std::make_unique<SliderInt>(&leafStride, "Leaf Stride", 0, 10));
	sliderLevels->setVisibility(false);
	slider->setVisibility(false);
	text->setVisibility(true);

	auto animateLoading = [text](std::string_view str, float deltaTime)
		{
			char frames[4] = { '\\', '|', '/', '-' };

			static unsigned int frame = 0;
			static float timer = .5f;
			timer = timer - deltaTime;

			if (timer < 0)
			{
				frame = (frame + 1) % 4;
				timer = .5f;
			}

			std::string s = "";
			s.push_back(frames[frame]);
			s += " ";
			s += str;

			text->str = s;
		};

	window.setKeyCallback([&window](int key, int scancode, int action, int mods)
		{
			if (key == GLFW_KEY_Q && action == GLFW_PRESS)
			{
				window.setShouldClose(true);
			}
		});

	window.setReshapeCallback([](int w, int h)
		{
			glViewport(0, 0, w, h);
			camera.onWindowResize(static_cast<float>(w), static_cast<float>(h));
			shaderManager.uploadCameraUniforms(camera, "view", "projection", "viewPosition");
		});

	glClearColor(0.72f, 0.75f, 0.92f, 1.0f);

	int lock = 0;
	std::thread setupThread(generatePlants);
	lock = 1;

	setupScene();

	while (!window.shouldClose())
	{
		float deltaTime = window.deltaTime();
		DebugHandler::instance().tick(deltaTime);

		int levelsTemp = levels;
		int leafStrideTemp = leafStride;

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if (!isInitialized.load())
		{
			animateLoading("Loading...", window.deltaTime());
		}

		else
		{
			if (setupThread.joinable())
			{
				setupThread.join();
				std::cout << "joined\n";
				text->setVisibility(false);
				slider->setVisibility(true);
				sliderLevels->setVisibility(true);
				plant.initialize();
				flower.initialize();
			}

			display(window, deltaTime);
		}

		ui.renderFrame(window);
		window.pollEvents();
		window.swapBuffers();

		if (levels != levelsTemp) trees->changeLevels(levels);
		if (leafStride != leafStrideTemp) trees->changeLeafStride(leafStride);
	}
	if (setupThread.joinable())
	{
		setupThread.join();
	}
	return 0;
}

GLuint loadTexture(const char* fileName)
{
	GLuint imageID;
	ilGenImages(1, &imageID);
	ilBindImage(imageID);
	ilEnable(IL_ORIGIN_SET);
	ilOriginFunc(IL_ORIGIN_LOWER_LEFT);
	if (!ilLoadImage((ILstring)fileName))
	{
		std::cout << "Couldn't load the following texture file: " << fileName << "\n";
		ilDeleteImages(1, &imageID);
		return 0;
	}
	ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);
	GLuint tid;
	glGenTextures(1, &tid);
	glBindTexture(GL_TEXTURE_2D, tid);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT), 0, GL_RGBA, GL_UNSIGNED_BYTE, ilGetData());
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
	ilDeleteImages(1, &imageID);
	return tid;
}

void initialize()
{
	loading = true;

	baseShader = shaderManager.addShader("shader.vert", "shader.frag");
	plantShader = shaderManager.addShader("plant.vert", "plant.frag");
	shaderManager.useShader(baseShader);

	glClearColor(0.72f, 0.75f, 0.92f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	Light light
	{
		.position = glm::vec3(20.0f, 30.0f, 20.0f),
		.diffuse = glm::vec3(1.0f),
		.ambient = glm::vec3(0.35f),
		.specular = glm::vec3(1.0f)
	};

	shaderManager.all().setLight(light);
	shaderManager.all().setMat3("texture", glm::mat3(1.0f));
	shaderManager.all().setInt("diffuseMap", 0);
	shaderManager.all().setInt("specularMap", 0);

	shaderManager.useShader(baseShader);

	ground.ambient = glm::vec3(1.0f);
	ground.diffuse = glm::vec3(0.247f, 0.56, 0.043f);
	ground.specular = glm::vec3(0.5f);
	ground.shininess = 15.0f;
	ground.diffuseMap = loadTexture("grass_diff.jpg");
	ground.specularMap = loadTexture("grass_spec.png");

	bark.ambient = glm::vec3(1.0f);
	bark.diffuse = glm::vec3(0.8f);
	bark.specular = glm::vec3(1.0f);
	bark.shininess = 30.0f;
	bark.diffuseMap = loadTexture("bark_diff.png");
	bark.specularMap = loadTexture("bark_spec.png");

	leaves.ambient = glm::vec3(0.06f, 0.5f, 0.1f);
	leaves.diffuse = glm::vec3(0.06f, 0.8f, 0.1f);
	leaves.specular = glm::vec3(1.0f);
	leaves.shininess = 10.0f;
	leaves.diffuseMap = loadTexture("leafDiff.png");
	leaves.specularMap = loadTexture("leafSpec.png");

	glViewport(0, 0, 1000, 700);

	camera.changeViewDistance(10);
	camera.setViewAngleAroundTarget(glm::radians(30.f), glm::radians(10.f));
	camera.onWindowResize(static_cast<float>(1000), static_cast<float>(700));
	camera.reComputeView();
	shaderManager.uploadCameraUniforms(camera, "view", "projection", "viewPosition");
	trees = std::make_unique<TreeManager>(Mesh{ Plane{}, 0.5f }, bark, bark, shaderManager.getShader(baseShader).value(), shaderManager.getShader(baseShader).value());

	trees->setUpTree(glm::scale(glm::mat4(1), glm::vec3(0.18f,0.18f,0.18f)), 5, 2, 30.5,1.f);
	//trees->setUpTree(glm::mat4(1), 2, 2, 30.5,1.f);
	trees->initialize();
	leafMesh.initialize();

	plant.initializeNoise();
	plant.bindNoise("noiseTexture", 2, shaderManager.getShader(plantShader).value());

	trees->setCount(6);
	trees->setSpacing(20);
	trees->setPosJitter(10.f);
	trees->setRotJitter(0.9f);
	trees->reloadTreeInstances();

}


void setupScene()
{
	plant.addComponent(Mesh{ "grass.obj" }, glm::translate(glm::mat4(1.0f), { 0,0,0 }) * glm::scale(glm::mat4(1.f), glm::vec3(0.5, .3, .5)), ground, shaderManager.getShader(plantShader).value());

	glm::mat4 flowerTransform{ 1 };
	Material stem{ .ambient = glm::vec3{1.f}, .diffuse = glm::vec3{0.23f, 0.58f, 0.18 }, .specular = glm::vec3(0.4), .shininess = 1, .diffuseMap = loadTexture("stemDiff.png") };
	Material flowerMat{ .ambient = glm::vec3{1.f}, .diffuse = glm::vec3{1,.2,.2},.specular = glm::vec3(0.4), .shininess = 1, .diffuseMap = loadTexture("flowerPetal.png") };
	Material bud{ .ambient = glm::vec3{1.f}, .diffuse = {1, 1, 0} };

	flower.addComponent(Mesh{ "stem.obj" }, flowerTransform, stem, shaderManager.getShader(plantShader).value());

	ModelComponent petalComponent(Mesh{ "petals.obj" }, flowerTransform, flowerMat, shaderManager.getShader(plantShader).value());

	petalComponent.colors.emplace_back(1.f, .5f, 0.f);
	petalComponent.colors.emplace_back(.6, .6, 1);
	petalComponent.colors.emplace_back(1, 0.5, 0.5);
	petalComponent.colors.emplace_back(1, 1, 0);

	flower.addComponent(petalComponent);
	flower.addComponent(Mesh{ "bud.obj" }, flowerTransform, bud, shaderManager.getShader(plantShader).value());
	flower.addComponent(Mesh{ "leaf1.obj" }, flowerTransform, leaves, shaderManager.getShader(plantShader).value());
	flower.addComponent(Mesh{ "leaf2.obj" }, flowerTransform, leaves, shaderManager.getShader(plantShader).value());
	flower.addComponent(Mesh{ "leaf3.obj" }, flowerTransform, leaves, shaderManager.getShader(plantShader).value());

	plant.unitize();
	flower.unitize();

	groundPlane.addComponent(plane, glm::mat4(1), ground, shaderManager.getShader(baseShader).value());
	groundPlane.addInstanceTransform(glm::translate(glm::mat4(1), { 0,0,0 }) * glm::scale(glm::mat4(1.f), glm::vec3{ 10 }));
	groundPlane.initialize();
}

void generatePlants()
{
	auto now = std::chrono::steady_clock::now();

	plant.makeFractal(7, 7, 18);
	//flower.makeFractal(7, 4, 18);

	plant.arrangePlants(50, 50, 0.25);
	flower.arrangePlants(20, 20, 0.6);
	auto duration = std::chrono::steady_clock::now() - now;
	auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
	auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);

	std::cout << "Setup time ms: " << milliseconds.count() << "\n";
	std::cout << "Setup time s: " << seconds.count() << "\n";

	isInitialized.store(true);
}

void display(Window& window, float deltaTime)
{
	static float angle = 0.f;

	Plant::tick(deltaTime * 0.5f);
	angle += cameraSpeed * deltaTime;

	shaderManager.useShader(baseShader);
	camera.setViewAngleAroundTarget(glm::radians(30.f), glm::radians(angle));
	camera.reComputeView();
	camera.setUniformPos("viewPosition", shaderManager.getShader(baseShader).value());
	camera.setUniformView("view", shaderManager.getShader(baseShader).value());

	if (auto lock = shaderManager.getShader(baseShader)->lock())
	{
		lock->setMaterial(ground);
	}
	{
		shaderManager.getShader(baseShader)->lock()->setMat3("texture", glm::mat3(50.f));
	}
	groundPlane.draw();

	shaderManager.useShader(plantShader);
	camera.setUniformPos("viewPosition", shaderManager.getShader(plantShader).value());
	camera.setUniformView("view", shaderManager.getShader(plantShader).value());

	plant.updateNoiseTexture();

	plant.draw();
	flower.draw();

	trees->draw();
	//trees->tick(deltaTime);
}