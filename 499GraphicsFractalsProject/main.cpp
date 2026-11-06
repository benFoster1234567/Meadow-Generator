#define ENSURE_GPU_RENDERED
//#define LOG_DEBUG_INFO

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

#include "GLDebugHandler.h"

#include "Window.h"
#include "Mesh.h"

#include "Ui.h"
#include "Plant.h"

#include "TreeController.h"
#include "Camera.h"
#include "ShaderManager.h"

#include "TreeItemPicker.h"

#include <IL/il.h>
#include <IL/ilu.h>
#include <IL/ilut.h>

#include <thread>

Material ground, bark, leaves;
Mesh plane(Plane{}, 10.f);

Camera camera{ 1000,700 };
ShaderManager shaderManager{};

ShaderManager::ShaderIndex leavesShader, barkShader, plantShader, instancedShader, leafShader, baseShader, pickerShader;

Plant plant{};
Plant flower{};
Plant groundPlane{};

TreeItemPicker treeItems{};

GpuMesh testMesh(Mesh{ Cube{}, 10.f });

constexpr size_t TreeVar = 3;

float cameraSpeed{ 0.f };

InstancedMesh leafMesh{ Mesh{Plane{}, 0.5f} };

std::unique_ptr<TreeManager> trees{};

bool loading = false;

std::unique_ptr<RenderedObject> newGround{};

//SIMD?
#define GLM_FORCE_INTRINSICS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES

/*

TODO:
- refactor code to oop design ~

BackBurner:
- make tree generation thread safe
- do heavy stuff on a background thread

*/

std::atomic<bool> isInitialized{ false };
int levels = 5;
float deltaX{}, stride{};
GLuint loadTexture(const char* fileName);
void initialize();
void setupScene();
void generatePlants();
void display(Window& window, float deltaTime);
void setUpTreeControls(ImGuiUi& ui, size_t defaultN, float defaultStride, glm::vec2 defaultDelta, size_t defaultLevels);

bool enableApples{ false };

void changeApples()
{
	treeItems.changePositions(trees->getAppleOrigins());
	treeItems.createKinematicRigidbody(glm::vec3(0, -1.f, 0), glm::vec3(50.f, 1.f, 50.f));

}

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
	DebugHandler::instance().setTypeRecord(DebugMsgType::GLStatus, true);
	DebugHandler::instance().setTypeRecord(DebugMsgType::Default, true);

	ilInit();

	initialize();

	std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
	std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;

	int number = 0;
	ImGuiUi ui;

	ui.setup(window);

	setUpTreeControls(ui, 10, 1.5f, { 70.f,1.5f }, 7);

	int leafStride = 0;

	auto* text = static_cast<Text*>(ui.addElement(std::make_unique<Text>("")));

	auto* slider = ui.addElement(std::make_unique<SliderFloat>(cameraSpeed, "Camera Orbit Speed", 0.f, 20.f, std::function([](float speed) { cameraSpeed = speed; })));

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
			treeItems.onResize(w, h);
			shaderManager.uploadCameraUniforms(camera, "view", "projection", "viewPosition");
		});

	window.setMouseCallback([](int button, int action, int mods, int mousex, int mousey)
		{
			treeItems.onClick(mousex, mousey, camera);
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

		if (enableApples)
		{
			treeItems.enable();
		}
		else
		{
			treeItems.disable();
		}

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
				plant.initialize();
				flower.initialize();
			}

			display(window, deltaTime);
		}

		ui.renderFrame(window);
		window.pollEvents();
		window.swapBuffers();

	}
	if (setupThread.joinable())
	{
		setupThread.join();
	}
	return 0;
}

void setUpTreeControls(ImGuiUi& ui, size_t defaultN, float defaultStride, glm::vec2 defaultDelta, size_t defaultLevels)
{
	ui.addElement(std::make_unique<SliderFloat>(defaultDelta.x, "deltaX", 0.f, 1000.f, std::function{
		[](float dx)
		{
			trees->setDeltaX(dx);
			trees->refreshTrees();
			changeApples();
		} }));

	ui.addElement(std::make_unique<SliderFloat>(defaultDelta.y, "deltaZ", 0.f, 100.f, std::function{
		[](float dz)
		{
			trees->setDeltaZ(dz);
			trees->refreshTrees();
			changeApples();

		} }));

	ui.addElement(std::make_unique<SliderFloat>(defaultStride, "Fractal Stride", 0.f, 5.f,
		[](float s)
		{
			trees->setStride(s);
			trees->refreshTrees();
			changeApples();

		}));

	ui.addElement(std::make_unique<SliderInt>(defaultLevels, "Level", 0, 15,
		[](int l)
		{
			trees->setLevels(l);
			trees->refreshTrees();
			changeApples();

		}));

	ui.addElement(std::make_unique<SliderInt>(defaultN, "N", 0, 15,
		[](int n)
		{
			trees->setN(n);
			trees->refreshTrees();
			changeApples();

		}));

	ui.addElement(std::make_unique<CheckBox>("Enable Apple Picking", [](bool on) {enableApples = on;}));
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
	
	pickerShader = shaderManager.addShader("pickingShader.vert", "pickingShader.frag");
	baseShader = shaderManager.addShader("shader.vert", "shader.frag");
	instancedShader = shaderManager.addShader("shaderInst.vert", "shader.frag");
	plantShader = shaderManager.addShader("plant.vert", "plant.frag");
	leafShader = shaderManager.addShader("plant.vert", "leafShader.frag");

	shaderManager.useShader(instancedShader);

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
	shaderManager.all().setInt("specularMap", 1);

	shaderManager.useShader(instancedShader);

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

	newGround = std::make_unique<RenderedObject>(Mesh(Plane{}, 100.f), shaderManager.getShader(baseShader).value(), ground);
	newGround->initialize();

	testMesh.initialize();

	std::vector<glm::mat4> ts{};
	ts.push_back(glm::translate(glm::mat4(1), glm::vec3(0, 7, 0)));

	Mesh appleMesh("Apple.obj");
	appleMesh.unitize({appleMesh}, true);
	treeItems.initialize(1000, 700
		, std::make_shared<GpuMesh>(appleMesh)
		, leaves
		, shaderManager.getShader(baseShader).value().lock() 
		, shaderManager.getShader(pickerShader).value().lock()  //sorry for this
		, ts, SphereCollider{.radius = .3f}, 1.f);

	treeItems.disable();

	treeItems.setScale(glm::vec3{ .5f });
	trees = std::make_unique<TreeManager>(Mesh{ Leaf{} }, bark, bark, shaderManager.getShader(baseShader).value(), shaderManager.getShader(leafShader).value());

	leafMesh.initialize();

	Plant::initializeNoise();
	plant.bindNoise("noiseTexture", 2, shaderManager.getShader(plantShader).value());

	trees->setCount(10);
	trees->setSpacing(10);
	trees->setPosJitter(10.f);
	trees->setRotJitter(0.9f);
	trees->reloadTreeInstances();
	trees->initialize();
	trees->refreshTrees(glm::scale(glm::mat4(1), glm::vec3(0.18f, 0.18f, 0.18f)), 5, 11, 80.f, 1.05f);
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
	plant.setTransform(glm::scale(glm::mat4(1), glm::vec3(.5f, .5f, .5f)));

	treeItems.createKinematicRigidbody(glm::vec3(0,-1.f,0), glm::vec3(50.f, 1.f, 50.f));

	groundPlane.addComponent(plane, glm::mat4(1), ground, shaderManager.getShader(instancedShader).value());
	groundPlane.addInstanceTransform(glm::translate(glm::mat4(1), { 0,0,0 }) * glm::scale(glm::mat4(1.f), glm::vec3{ 10 }));
	groundPlane.initialize();
}

void generatePlants()
{
	auto now = std::chrono::steady_clock::now();

	plant.makeFractal(7, 7, 18);
	flower.makeFractal(7, 5, 18);

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

	camera.setViewAngleAroundTarget(glm::radians(30.f), glm::radians(angle));
	camera.reComputeView();
	shaderManager.uploadCameraUniforms(camera, "view", "projection", "viewPosition");
	shaderManager.useShader(instancedShader);

	if (auto lock = shaderManager.getShader(instancedShader)->lock())
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

	//Sidebar: add SIMD support for this? - OpenMP?
	trees->draw();
	Plant::updateNoiseTexture();
	plant.draw();
	flower.draw();

	treeItems.draw(camera);
	treeItems.runSim(deltaTime);

}