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

#include "TreeItemPicker.h"

#include <IL/il.h>
#include <IL/ilu.h>
#include <IL/ilut.h>

#include <thread>
#include "Renderer.h"
#include "SceneBuilder.h"

Material ground, bark, leaves;
Material stemMat;
Material flowerMat;
Material budMat;

Camera camera{ 1000,700 };

TreeItemPicker treeItems{};

float cameraSpeed{ 0.f };


std::unique_ptr<Renderer> renderer{};
std::unique_ptr<TreeManager> trees{};

bool loading = false;

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
		});

	window.setMouseCallback([](int button, int action, int mods, int mousex, int mousey)
		{
			treeItems.onClick(mousex, mousey, camera);
		});

	glClearColor(0.72f, 0.75f, 0.92f, 1.0f);

	int lock = 0;
	std::thread setupThread(generatePlants);
	lock = 1;

	//setupScene();

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
				//plant.initialize();
				//flower.initialize();
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

	ui.addElement(std::make_unique<CheckBox>("Enable Apple Picking", [](bool on) {enableApples = on; }));
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

static void assembleMaterials()
{

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

	stemMat = { .ambient = glm::vec3{1.f}, .diffuse = glm::vec3{0.23f, 0.58f, 0.18f}, .specular = glm::vec3(0.4f), .shininess = 1.f, .diffuseMap = loadTexture("stemDiff.png") };
	flowerMat = { .ambient = glm::vec3{1.f}, .diffuse = glm::vec3{1.f, 0.2f, 0.2f}, .specular = glm::vec3(0.4f), .shininess = 1.f, .diffuseMap = loadTexture("flowerPetal.png") };
	budMat = { .ambient = glm::vec3{1.f}, .diffuse = glm::vec3{1.f, 1.f, 0.f} };

}

void initialize()
{

	assembleMaterials();
	Light light
	{
		.position = glm::vec3(20.0f, 30.0f, 20.0f),
		.diffuse = glm::vec3(1.0f),
		.ambient = glm::vec3(0.35f),
		.specular = glm::vec3(1.0f)
	};

	SceneBuilderConfig scene
	{
		.ground = ground,
		.bark = bark,
		.leaves = leaves,
		.treeleaf = Mesh(Leaf{}),
		.grass = Mesh{"grass.obj"},
		.shader = {"shader.vert","shader.frag"},
		.plantShader = {"plant.vert", "plant.frag"},
		.instancedShader = {"shaderInst.vert", "shaderInst.frag"},
		.leafShader = {"plant.vert", "leafShader.frag"},
		.light = light,
		.flowerSettings =
		{
			.stemMat = stemMat,
			.flowerMat = flowerMat,
			.budMat = budMat,
			.petals = Mesh{"petals.obj"},
			.stem = Mesh{"stem.obj"},
			.bud = Mesh{"bud.obj"},
			.leaf1 = Mesh{"leaf1.obj"},
			.leaf2 = Mesh{"leaf2.obj"},
			.leaf3 = Mesh{"leaf3.obj"},
		}
	};

	SceneBuilder builder(std::move(scene));
	
	auto result = builder.BuildScene();

	trees = std::move(result.trees);
	renderer = std::move(result.renderer);

	glClearColor(0.72f, 0.75f, 0.92f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);


	glViewport(0, 0, 1000, 700);

	camera.changeViewDistance(10);
	camera.setViewAngleAroundTarget(glm::radians(30.f), glm::radians(10.f));
	camera.onWindowResize(static_cast<float>(1000), static_cast<float>(700));
	camera.reComputeView();
}

void generatePlants()
{
	auto now = std::chrono::steady_clock::now();

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

	angle += cameraSpeed * deltaTime;

	camera.setViewAngleAroundTarget(glm::radians(30.f), glm::radians(angle));
	camera.reComputeView();
	renderer->updateNoise(deltaTime);
	renderer->draw(camera);

	//Plant::updateNoiseTexture();
	//plant.draw();
	//flower.draw();

	//treeItems.draw(camera);
	//treeItems.runSim(deltaTime);

}