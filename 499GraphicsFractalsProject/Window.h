#pragma once

#include <GLFW/glfw3.h>
#include <string>
#include <stdexcept>
#include <functional>
class Window
{
private:


	unsigned int m_width{};
	unsigned int m_height{};
	float m_time{};
	std::string m_title{};
	
	GLFWwindow* m_window{nullptr};

	std::function<void(int, int, int, int)> m_onKey{};
	std::function<void(int, int)> m_onReshape{};

	friend class ImGuiUi;
public:
	Window() = default;

	Window(unsigned int width, unsigned int height)
		: m_width{width}, m_height{height}
	{

		m_window = glfwCreateWindow(width, height, "Hello World", NULL, NULL);
		if (!m_window)
		{
			glfwTerminate();
			throw std::runtime_error("glfw failed to create window!\n");
		}

		glfwWindowHint(GLFW_SAMPLES, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glEnable(GL_MULTISAMPLE);
		glfwSetWindowUserPointer(m_window, this);
		glfwSetKeyCallback(m_window, glfwKeyCallback);
		glfwSetFramebufferSizeCallback(m_window, glfwReshapeCallback);
		glfwSetErrorCallback(glfwErrorCallback);
		glfwMakeContextCurrent(m_window);
		glfwSwapInterval(1);
		m_time = static_cast<float>(glfwGetTime());
	}
	
	~Window()
	{
		glfwTerminate();
	}

	//called every frame
	float deltaTime()
	{
		float curTime = glfwGetTime();
		float dTime = curTime - m_time;
		m_time = curTime;
		return dTime;
	}

	void setShouldClose(bool shouldClose)
	{
		glfwSetWindowShouldClose(m_window, shouldClose ? GLFW_TRUE : GLFW_FALSE);
	}

	static void glfwKeyCallback(GLFWwindow* glfwWindow, int key, int scancode, int action, int mods)
	{
		Window*  window = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
		if (!window) return;

		window->m_onKey(key, scancode, action, mods);
		
	}

	static void glfwErrorCallback(int error, const char* description)
	{
		fprintf(stderr, "GLFW Error %d: %s\n", error, description);
	}


	static void glfwReshapeCallback(GLFWwindow* glfwWindow, int width, int height)
	{
		Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
		if (!window) return;

		window->m_width = width;
		window->m_height = height;
		window->m_onReshape(width, height);
	}

	void setReshapeCallback(std::function<void(int,int)> func)
	{
		m_onReshape = func;
	}

	void setKeyCallback(std::function<void(int, int, int, int)> func)
	{
		m_onKey = func;
	}

	bool shouldClose() const
	{
		return glfwWindowShouldClose(m_window);
	}

	void makeContextCurrent()
	{
		glfwMakeContextCurrent(m_window);
	}

	void swapBuffers()
	{
		glfwSwapBuffers(m_window);
	}

	void pollEvents()
	{
		glfwPollEvents();
	}

};