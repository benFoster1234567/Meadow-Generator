#pragma once
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include "Window.h"

#define GL_SILENCE_DEPRECATION

struct ImGuiElementBase
{
	virtual void render() = 0;
};

struct Button : public ImGuiElementBase
{
	std::string m_label{""};
	std::function<void()> m_func{};

	Button(std::string_view label, std::function<void()> func)
		: m_label(label), m_func(func) 
	{
	}

	void render() override
	{
		if (ImGui::Button(m_label.c_str()))
		{
			m_func();
		}
	}

};

class SliderInt : public ImGuiElementBase
{
	std::string m_label{ "int slider" };
	int m_min{0};
	int m_max{1};
	int* m_value{ nullptr };

public:
	SliderInt(int* value, std::string_view label, int min, int max)
		: m_label(label), m_min(min), m_max(max), m_value{ value } 
	{
	}

	void render() override
	{
		int i{};
		ImGui::SliderInt(m_label.c_str(), !m_value? &i : m_value, m_min, m_max);
	}

};

struct Text : public ImGuiElementBase
{
	std::string str{ "" };

	void render() override
	{
		ImGui::Text(str.c_str());
	}
};


class ImGuiUi
{
private:
	bool show_demo_window{ false };
	bool show_another_window{ false };
	std::vector<std::unique_ptr<ImGuiElementBase>> elements{};
	
	std::string m_windowTitle{ "imgui window" };

	friend class ImGuiUiBuilder;

public:

	ImGuiUi() = default;

	ImGuiUi(const std::string& windowTitle) : m_windowTitle{ windowTitle } {}

	void setup(Window& window)
	{
		float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());
		const char* glsl_version = "#version 430";

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

		ImGui::StyleColorsClassic();

		ImGuiStyle& style = ImGui::GetStyle();
		style.ScaleAllSizes(main_scale);
		style.FontScaleDpi = main_scale;

		ImGui_ImplGlfw_InitForOpenGL(window.m_window, true);
		ImGui_ImplOpenGL3_Init(glsl_version);
	}

	void addElement(std::unique_ptr<ImGuiElementBase> element)
	{
		elements.push_back(std::move(element));
	}

	void renderFrame(Window& window)
	{
		if (glfwGetWindowAttrib(window.m_window, GLFW_ICONIFIED) != 0)
		{
			ImGui_ImplGlfw_Sleep(10);
			return;
		}

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		{
			ImGui::Begin(m_windowTitle.c_str());

			ImGuiIO& io = ImGui::GetIO();
			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);


			for (auto& elem : elements)
			{
				elem->render();
			}

			ImGui::End();
			/*static float f = 0.0f;
			static int counter = 0;

			ImGui::Begin(m_windowTitle.c_str());

			ImGui::Text("This is some useful text.");
			ImGui::Checkbox("Demo Window", &show_demo_window);
			ImGui::Checkbox("Another Window", &show_another_window);

			ImGui::SliderFloat("float", &f, 0.0f, 1.0f);

			if (ImGui::Button("Button"))
				counter++;
			ImGui::SameLine();
			ImGui::Text("counter = %d", counter);
			ImGui::End();*/

		}

		if (show_demo_window)
			ImGui::ShowDemoWindow(&show_demo_window);

		ImGui::Render();

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}

	void shutdown()
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}
};
