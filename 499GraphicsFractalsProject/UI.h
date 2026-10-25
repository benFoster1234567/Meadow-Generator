#pragma once
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include "Window.h"

#define GL_SILENCE_DEPRECATION

struct ImGuiElementBase
{
	bool m_isVisible = true;
	virtual void render() = 0;
	virtual bool visible() const
	{
		return m_isVisible;
	}
	virtual void setVisibility(bool v)
	{
		m_isVisible = v;
	}
};

struct Button : public ImGuiElementBase
{
	std::string m_label{ "" };
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
	int m_min{ 0 };
	int m_max{ 1 };
	int* m_value{ nullptr };

public:
	SliderInt(int* value, std::string_view label, int min, int max)
		: m_label(label), m_min(min), m_max(max), m_value{ value }
	{
	}

	void render() override
	{
		int i{};
		ImGui::SliderInt(m_label.c_str(), !m_value ? &i : m_value, m_min, m_max);
	}

};

class SliderFloat : public ImGuiElementBase
{
	std::string m_label{ "float slider" };
	float m_min{ 0 };
	float m_max{ 1 };
	float* m_value{ nullptr };

public:
	SliderFloat(float* value, std::string_view label, float min, float max)
		: m_label(label), m_min(min), m_max(max), m_value{ value }
	{
	}

	void render() override
	{
		float i{};
		ImGui::SliderFloat(m_label.c_str(), !m_value ? &i : m_value, m_min, m_max, "%.3f");
	}

};

struct Text : public ImGuiElementBase
{
	std::string str{ "" };

	Text(std::string_view s) : str{ s } {}

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

	ImGuiElementBase* addElement(std::unique_ptr<ImGuiElementBase> element)
	{
		elements.push_back(std::move(element));
		return elements.back().get();
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
				if (elem->visible())
				{
					elem->render();
				}
			}

			ImGui::End();
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
