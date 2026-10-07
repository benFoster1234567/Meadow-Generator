#include <GL/glew.h>
#include <GLFW/glfw3.h>


#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Window.h"
#include "OpenGL.h"
#include "Mesh.h"
#include "Tree.h"
#include "Ui.h"


const char* v_shader = R"(
#version 430 core

layout (location = 0) in vec3 in_Position;
layout (location = 1) in vec3 in_Normal;
layout (location = 2) in vec2 in_TexCoord0;

layout (location = 0) out vec3 out_Position;
layout (location = 1) out vec3 out_Normal;
layout (location = 2) out vec2 out_TexCoord0;
layout (location = 3) out vec3 out_ViewPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 texture;

void main() {
	out_Position = vec3(model * vec4(in_Position, 1.0));
	out_Normal = mat3(transpose(inverse(model))) * in_Normal;
	out_TexCoord0 = vec2(texture * vec3(in_TexCoord0, 1.0));
	gl_Position = projection * view * model * vec4(in_Position, 1.0);
}
)";

const char* f_shader = R"(
#version 430 core

struct Material {
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
layout (location = 3) in vec3 in_ViewPos;

layout (location = 0) out vec4 out_Color;

uniform sampler2D diffuseMap;
uniform sampler2D specularMap;

uniform vec3 viewPosition;
uniform Light light;
uniform Material material;

void main() {
	vec3 normal = gl_FrontFacing ? normalize(in_Normal) : -normalize(in_Normal);
	vec3 lightDir = normalize(light.position);
	//vec3 lightDir = normalize(light.position - in_Position);
	float diff = max(dot(normal, lightDir), 0.0);
	float spec = 0.0;
	if (diff > 0.0) {
		vec3 viewDir = normalize(viewPosition - in_Position);
		//vec3 reflectDir = reflect(-lightDir, viewDir);
		//spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
		vec3 halfDir = normalize(lightDir + viewDir);
		spec = pow(max(dot(halfDir, normal), 0.0), material.shininess);
	}
	out_Color = vec4(
		(
			light.ambient * material.ambient +
			light.diffuse * diff * material.diffuse
		) * vec3(texture(diffuseMap, in_TexCoord0)) +
		light.specular * spec * vec3(texture(specularMap, in_TexCoord0)) * material.specular,
		1.0
	);
}
)";

GLuint program;
Material ground, bark, leaves;
GLfloat cameraYaw = 0.25f * glm::pi<GLfloat>();
GLfloat cameraPitch = -0.4f;

int lx, ly;

int level = 12;
int prevLevel = level;

GLfloat nextFloat() {
	return rand() / (GLfloat)RAND_MAX;
}

Mesh cube(Cube{}, 0.5f);
Mesh plane(Plane{}, 60.f);

Branch mybranch{ 1.f, 8 };
Tree myTree{12, 0.2f};

void growBranch(int times)
{
	glm::mat4 transform{ 1.f };
	for (int i = 0; i < times; i++)
	{
		transform *= glm::scale(glm::mat4(1), {0.9,0.9,1})
			* glm::translate(glm::mat4{ 1.f }, { 0,1,-0.5 })
			* glm::rotate(glm::mat4(1), 0.2f, { 1,0,0 });
		
		mybranch.addLayer(transform, .1, 2);
	}
}

static void initialize()
{
	//growBranch(5);
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
    ground.diffuse = glm::vec3(0.8f);
    ground.specular = glm::vec3(0.2f);
    ground.shininess = 200.0f;
    ground.diffuseMap = loadTexture("grass_diff.jpg");
    ground.specularMap = loadTexture("grass_spec.png");

	bark.ambient = glm::vec3(1.0f);
	bark.diffuse = glm::vec3(0.8f);
	bark.specular = glm::vec3(0.4f);
	bark.shininess = 30.0f;
	bark.diffuseMap = loadTexture("bark_diff.png");
	bark.specularMap = loadTexture("bark_spec.png");

	leaves.ambient = glm::vec3(0.06f, 0.5f, 0.1f);
	leaves.diffuse = glm::vec3(0.06f, 0.5f, 0.1f);
	leaves.specular = glm::vec3(0.8f);
	leaves.shininess = 80.0f;
	leaves.diffuseMap = loadTexture("leaf_diff.png");
	leaves.specularMap = loadTexture("leaf_spec.png");

	glm::mat4 local = glm::translate(glm::mat4(1.0f), glm::vec3(0, -3.0f, 0)) *
		glm::rotate(glm::mat4(1.0f), nextFloat() * 2.0f * glm::pi<GLfloat>(), glm::vec3(0.0f, 1.0f, 0.0f)) *
		glm::rotate(glm::mat4(1.0f), glm::pi<GLfloat>() / 2.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	
	myTree.reconstruct(local, level, 0, 0, 3.0f, 5.5f);

	//mybranch.initialize();
}

void branch(glm::mat4 local, size_t n, GLfloat deltaX, GLfloat deltaY, GLfloat girth, GLfloat stride) {
	for (size_t s = 0; s < n; s++) {
		GLfloat g = girth - girth * s / (GLfloat)n * 0.5f;
		GLfloat st = stride - stride * s / (GLfloat)n * 0.25f;
		local *= glm::rotate(glm::mat4(1.0f), 0.1f, glm::vec3(0.0f, 0.0f, 1.0f));
		
		setMat4("model", local *
			glm::scale(glm::mat4(1.0f), glm::vec3(g, g, st)) *
			glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -0.5f)) *
			glm::rotate(glm::mat4(1.0f), glm::pi<GLfloat>() / 2.0f, glm::vec3(1.0f, 0.0f, 0.0f)), 
			program
		);

		setMaterial(bark, program);
		
		cube.draw();
		
		if (s > n / 2 && nextFloat() < 0.75f) {
			GLfloat theta = nextFloat() * glm::pi<GLfloat>();
			branch(local *
				glm::rotate(glm::mat4(1.0f), glm::pi<GLfloat>() / 2.0f, glm::vec3(1.0f, 0.0f, 0.0f)) *
				glm::rotate(glm::mat4(1.0f), theta, glm::vec3(0.0f, 1.0f, 0.0f)), n / 2, -deltaX, deltaY, g * 0.5f, st);
		}

		local *= glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -st));
		local *= glm::rotate(glm::mat4(1.0f), deltaX * 0.1f, glm::vec3(1.0f, 0.0f, 0.0f));
		local *= glm::rotate(glm::mat4(1.0f), deltaY * 0.05f, glm::vec3(0.0f, 1.0f, 0.0f));

		deltaX += (nextFloat() - nextFloat()) * nextFloat() * 2;
		deltaY += (nextFloat() - nextFloat()) * nextFloat() * 4;
	}
	if (n > 1) {
		GLfloat theta = nextFloat() * glm::pi<GLfloat>();
		branch(local *
			glm::rotate(glm::mat4(1.0f), glm::pi<GLfloat>() / 4.0f, glm::vec3(1.0f, 0.0f, 0.0f)), n * 2 / 3, -deltaX, deltaY, girth * 0.5f, stride * 0.75f);
		branch(local *
			glm::rotate(glm::mat4(1.0f), -glm::pi<GLfloat>() / 4.0f, glm::vec3(1.0f, 0.0f, 0.0f)), n * 2 / 3, -deltaX, deltaY, girth * 0.5f, stride * 0.75f);
	}
	else {
		GLfloat leafScale = nextFloat() * 2.0f + 1.0f;
		setMat4("model", local *
			glm::scale(glm::mat4(1.0f), glm::vec3(leafScale)) *
			glm::rotate(glm::mat4(1.0f), glm::pi<GLfloat>() / 2.0f, glm::vec3(1.0f, 0.0f, 0.0f)), program
		);
		setMaterial(leaves, program);
		cube.draw();
	}
}


static void display(Window& window)
{
	static float angle = 0.f;
	angle += 10.f * window.deltaTime();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(program);

	glm::vec4 eye = glm::rotate(glm::mat4{ 1 }, glm::radians(angle), { 0,1,0 }) * glm::vec4(0.0f, 20.0f, 60.0f, 1.0f);
	setMat4("view", glm::lookAt(
		glm::vec3(eye),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f)
	), program);
	setVec3("viewPosition", eye, program);
	// Ground
	setMat4("model", glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -4.0f, 0.0f)) * glm::scale(glm::mat4(1), { 10,10,10 }), program);
	setMat3("texture", glm::mat3(glm::scale(glm::mat4(1.0f), glm::vec3(12.0f))), program);
	setMaterial(ground, program);
	plane.draw();
	setMat3("texture", glm::mat3(glm::scale(glm::mat4(1.0f), glm::vec3(0.5f))), program);
	setMaterial(bark, program);
	srand(0x390F2A);
	size_t treeCount = 1;
	GLfloat theta = 2.0f * glm::pi<GLfloat>() * treeCount;
	GLfloat x = glm::cos(theta) * 20.0f;
	GLfloat z = glm::sin(theta) * 20.0f;
	GLfloat deltaX = nextFloat() * 0.3f - 0.2f;
	GLfloat deltaY = nextFloat() * 0.3f - 0.2f;
	glm::mat4 local = glm::translate(glm::mat4(1.0f), glm::vec3(0, -3.0f, 0)) *
		glm::rotate(glm::mat4(1.0f), nextFloat() * 2.0f * glm::pi<GLfloat>(), glm::vec3(0.0f, 1.0f, 0.0f)) *
		glm::rotate(glm::mat4(1.0f), glm::pi<GLfloat>() / 2.0f, glm::vec3(1.0f, 0.0f, 0.0f));

	if (prevLevel != level)
	{
		prevLevel = level;
		myTree.reconstruct(local, level, deltaX, deltaY, 3.1f, 2.5f);
	}

	setMat4("model", glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -4.0f, 0.0f)) * glm::scale(glm::mat4(1), { 1,1,1 }), program);
	myTree.draw();

	//branch(local, 20, deltaX, deltaY, 3.0f, 3.5f);
	//branch(local, level, 0, 0, 3.0f, 3.5f);

	//mybranch.draw();

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
	
	ImGuiUi ui;
	ui.setup(window);

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
			setMat4("projection", glm::perspective(glm::radians(100.0f), (GLfloat)w / h, 1.0f, 200.0f), program);
		});


	ui.addElement(std::make_unique<SliderInt>(SliderInt(&level, "Level", 1, 20)));
	
	cube.initialize();
	plane.initialize();
	glm::mat4 projection = glm::perspective(glm::radians(100.0f), 1000.0f / 700.0f, 1.0f, 200.0f);
	setMat4("projection", projection, program);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	//glEnable(GL_CULL_FACE);

	while (!window.shouldClose())
	{
		display(window);
		ui.renderFrame(window);
		window.pollEvents();
		window.swapBuffers();
	}

	return 0;
}