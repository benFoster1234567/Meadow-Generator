#include <GL/glew.h>
#include <GLFW/glfw3.h>


#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Window.h"
#include "OpenGL.h"
#include "Mesh.h"

#include "Ui.h"
#include "Plant.h"
#include "Plane.h"

#include <thread>
#include "TreeGenerator.h"
#include "TreeController.h"

const char* v_shader = R"(
#version 430 core

layout (location = 0) in vec3 in_Position;
layout (location = 1) in vec3 in_Normal;
layout (location = 2) in vec2 in_TexCoord0;
layout (location = 3) in mat4 in_InstanceData;

layout (location = 0) out vec3 out_Position;
layout (location = 1) out vec3 out_Normal;
layout (location = 2) out vec2 out_TexCoord0;
layout (location = 3) out vec3 out_color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 texture;

void main() 
{
	mat4 m = in_InstanceData;

	out_color = vec3(m[0].w, m[1].w, m[2].w);
	
	m[0].w = 0.0;
	m[1].w = 0.0;
	m[2].w = 0.0;
	m[3].w = 1.0;

	mat4 worldModel = m * model;

	vec3 worldPosition = vec3(worldModel * vec4(in_Position, 1.0));

	out_Position = worldPosition;

	//out_Normal = in_Normal;
	out_Normal = mat3(transpose(inverse(worldModel))) * in_Normal;
	//out_TexCoord0 = vec2(texture * vec3(in_TexCoord0, 1.0));
	out_TexCoord0 = in_TexCoord0;
	gl_Position = projection * view * vec4(worldPosition, 1.0);

}
)";

const char* f_shader = R"(
#version 430 core

struct Material 
{
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float shininess;
};

struct Light {
	vec3 position;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

layout (location = 0) in vec3 in_Position;
layout (location = 1) in vec3 in_Normal;
layout (location = 2) in vec2 in_TexCoord0;
layout (location = 3) in vec3 in_Color;

layout (location = 0) out vec4 out_Color;

uniform sampler2D diffuseMap;
uniform sampler2D specularMap;

uniform vec3 viewPosition;
uniform Light light;
uniform Material material;

vec3 overlay(vec3 base, vec3 blend)
{
    vec3 multiply = 2.0 * base * blend;
    vec3 screen   = 1.0 - 2.0 * (1.0 - base) * (1.0 - blend);
    return mix(multiply, screen, step(0.5, base));
}

float softLightComponent(float base, float detail) 
{
    if (detail <= 0.5) 
	{
        return base - (1.0 - 2.0 * detail) * base * (1.0 - base);
    } 
	else 
	{
        return base + (2.0 * detail - 1.0) * (sqrt(base) - base);
    }
}

vec3 blendSoftLight(vec3 baseColor, vec3 detailColor) 
{
    return vec3(
        softLightComponent(baseColor.r, detailColor.r),
        softLightComponent(baseColor.g, detailColor.g),
        softLightComponent(baseColor.b, detailColor.b)
    );
}

void main() 
{
	vec3 N = gl_FrontFacing ? normalize(in_Normal) : -normalize(in_Normal);
	
	vec3 L = normalize(light.position);
	vec3 E = normalize(viewPosition - in_Position);
	vec3 H = normalize(L + E);

	float Kd = max(dot(L, N), 0.0);
	float Ks = Kd > 0 ? pow(max(dot(N, H), 0.0), material.shininess) : 0.0;
	
	vec3 texColor = texture(diffuseMap, in_TexCoord0).rgb;
	vec3 c = in_Color.rgb;
	
	if (length(c) ==0)
	{
		c = texColor;
	}

	

	vec3 ambient = material.ambient * light.ambient * c;
	vec3 specular = Ks * light.specular * material.specular * vec3(texture(specularMap, in_TexCoord0));
	vec3 diffuse = light.diffuse * material.diffuse * c * Kd *2 ;

	out_Color = vec4((ambient + diffuse + specular), 1.0);
	//out_Color = vec4((ambient + diffuse), 1.0);
	//out_Color = gl_FrontFacing? vec4(1,1,1,1) : vec4(1,0,0,1);
	//out_Color = vec4(fract(in_TexCoord0.y), 0,0,1);
}
)";

GLuint program;
Material ground, bark, leaves;

Mesh plane(Plane{}, 60.f);

Mesh meshMesh("bunny.obj");

Plant plant{};
Plant flower{};
Plant groundPlane{};

Mesh testTree;

float cameraSpeed{ 0.f };

InstancedMesh meshy{};

InstancedMesh leafMesh{ Mesh{Plane{}, 0.5f}, 0 };

std::unique_ptr<TreeManager> trees{ nullptr };
bool loading = false;

static void initialize()
{
	loading = true;

	program = initializeShaders(v_shader, f_shader);
	glUseProgram(program);
	glClearColor(0.72f, 0.75f, 0.92f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	setMat3("texture", glm::mat3(1.0f), program);
	setVec3("light.position", glm::vec3(20.0f, 30.0f, 20.0f), program);
	setVec3("light.ambient", glm::vec3(0.35f), program);
	setVec3("light.diffuse", glm::vec3(1.0f), program);
	setVec3("light.specular", glm::vec3(1.0f), program);
	setInt("diffuseMap", 0, program);
	setInt("specularMap", 1, program);

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
	setMat4("projection", glm::perspective(glm::radians(60.0f), (GLfloat)1000 / 700, 1.0f, 200.0f), program);

	TreeGenerator t{ 4,16 };

	float deltaX = 10.5f;
	float stride = 1.5f;
	leafMesh.shaderProgram = program;
	t.generateExampleTree(glm::mat4(1), 6, 10, { 2,1,2 }, deltaX, 10.5f, stride, 5.0f, 30);
	leafMesh.changeInstances(t.getLeafInstances());

	testTree = t.getMesh();
	meshy.m_mesh = testTree;
	meshy.shaderProgram = program;

	meshy.addInstance(glm::translate(glm::mat4{ 1 }, glm::vec3{ 0,-4,0 }) * glm::scale(glm::mat4(1), glm::vec3(1)));
	//meshy.addInstance(glm::translate(glm::mat4{ 1 }, glm::vec3{ 0,-4,0 }) * glm::rotate(glm::mat4(1), -0.4f, glm::vec3(1, 0, 0)) * glm::scale(glm::mat4(1), glm::vec3(1)));

	trees = std::make_unique<TreeManager>(Mesh{ Plane{}, 0.5f }, bark, bark, program);

	trees->setUpTree(6);
	trees->initialize();
	leafMesh.initialize();
	meshy.initialize();

}



std::atomic<bool> isInitialized{ false };

static void setupScene()
{
	plant.addComponent(Mesh{ "grass.obj" }, glm::translate(glm::mat4(1.0f), { 0,0,0 }) * glm::scale(glm::mat4(1.f), glm::vec3(0.5, .3, .5)), ground, program);

	glm::mat4 flowerTransform = glm::translate(glm::mat4(1.0f), { 0,0,0 }) * glm::scale(glm::mat4(1.0f), glm::vec3(1.3, 1, 1.3));
	//0.7098f, 0.8235f, 0.6784f
	Material stem{ .ambient = glm::vec3{1.f}, .diffuse = glm::vec3{0.23f, 0.58f, 0.18 }, .specular = glm::vec3(0.4), .shininess = 1, .diffuseMap = loadTexture("stemDiff.png") };
	Material flowerMat{ .ambient = glm::vec3{1.f}, .diffuse = glm::vec3{1,.2,.2},.specular = glm::vec3(0.4), .shininess = 1, .diffuseMap = loadTexture("flowerPetal.png") };
	Material bud{ .ambient = glm::vec3{1.f}, .diffuse = {1, 1, 0} };

	flower.addComponent(Mesh{ "stem.obj" }, flowerTransform, stem, program);

	ModelComponent petalComponent(Mesh{ "petals.obj" }, flowerTransform, flowerMat, program);
	petalComponent.colors.emplace_back(1.f, .5f, 0.f);
	petalComponent.colors.emplace_back(.6, .6, 1);
	petalComponent.colors.emplace_back(1, 0.5, 0.5);
	petalComponent.colors.emplace_back(1, 1, 0);

	//flower.addComponent(Mesh{ "petals.obj" }, flowerTransform, flowerMat, program);
	flower.addComponent(petalComponent);
	flower.addComponent(Mesh{ "bud.obj" }, flowerTransform, bud, program);
	flower.addComponent(Mesh{ "leaf1.obj" }, flowerTransform, leaves, program);
	flower.addComponent(Mesh{ "leaf2.obj" }, flowerTransform, leaves, program);
	flower.addComponent(Mesh{ "leaf3.obj" }, flowerTransform, leaves, program);

	groundPlane.addComponent(plane, glm::mat4(1), ground, program);
	groundPlane.addInstanceTransform(glm::translate(glm::mat4(1), { 0,0,0 }) * glm::scale(glm::mat4(1.f), glm::vec3{ 10 }));
	groundPlane.initialize();
}

static void generatePlants()
{
	auto now = std::chrono::steady_clock::now();

	//plant.makeFractal(7, 6, 70, 3.f);
	//flower.makeFractal(5, 6, 80, 5, .5);

	plant.arrangePlants(10, 10, 1);

	auto duration = std::chrono::steady_clock::now() - now;
	auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
	auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);

	std::cout << "Setup time ms: " << milliseconds.count() << "\n";
	std::cout << "Setup time s: " << seconds.count() << "\n";

	isInitialized.store(true);
}

void changeTree(int llevels)
{
	TreeGenerator t{ 4,16 };
	float deltaX = 30.5f;
	float stride = 1.5f;
	t.generateExampleTree(glm::mat4(1), llevels, 10, { 2,1,2 }, deltaX, 10.5f, stride, 5.0f, 30);
	meshy.changeMesh(t.getMesh());
	leafMesh.changeInstances(t.getLeafInstances());
}

int levels = 5;

static void display(Window& window)
{
	static float angle = 0.f;
	float deltaTime = window.deltaTime();
	angle += cameraSpeed * deltaTime;

	glUseProgram(program);
	//plant.wigglePlants();
	//flower.wigglePlants();
	glm::vec4 eye = glm::rotate(glm::mat4{ 1 }, glm::radians(angle), { 0,1,0 }) * glm::vec4(0.0f, 40.0f, 60.0f, 1.0f);
	setMat4("view", glm::lookAt(
		glm::vec3(eye),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f)
	), program);
	setVec3("viewPosition", eye, program);

	setMaterial(ground, program);

	//setMat3("texture", glm::mat3(glm::scale(glm::mat4(1.0f), glm::vec3(12.0f))), program);

	groundPlane.draw();
	//flower.animateUpdate();
	flower.wigglePlants();
	plant.wigglePlants();
	plant.draw();
	flower.draw();
	setMaterial(bark, program);
	setMat3("texture", glm::mat3(10), program);
	//meshy.draw();
	//leafMesh.draw();

	trees->draw();
	//trees->tick(deltaTime);
}

int main(int argc, char** argv)
{
	if (!glfwInit()) return -1;
	Window window(1000, 700);
	window.makeContextCurrent();

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		std::cout << "GLEW Init failed!\n";
	}

	ilInit();

	initialize();


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
			setMat4("projection", glm::perspective(glm::radians(60.0f), (GLfloat)w / h, 1.0f, 200.0f), program);
		});

	glClearColor(0.72f, 0.75f, 0.92f, 1.0f);

	int lock = 0;
	std::thread setupThread(generatePlants);
	lock = 1;

	setupScene();



	while (!window.shouldClose())
	{
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

			display(window);
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
