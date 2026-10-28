#pragma once
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include "Window.h"

#define GL_SILENCE_DEPRECATION

class ImGuiElementBase
{
public:
	virtual ~ImGuiElementBase() = default;

	virtual void render() = 0;
	virtual bool visible() const
	{
		return m_isVisible;
	}
	virtual void setVisibility(bool v)
	{
		m_isVisible = v;
	}
protected:
	bool m_isVisible = true;
};

class Button : public ImGuiElementBase
{
public:
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

protected:
	std::string m_label{ "" };
	std::function<void()> m_func{};

};

template<class T>
class SliderBase : public ImGuiElementBase
{
public:
	static_assert(std::is_arithmetic_v<T>, "Template type T must be a number!");

	SliderBase(T value, std::string_view label, const T& min, const T& max, const std::function<void(T)>& onEvent)
		: m_label{ label }, m_value{ value }, m_min{ min }, m_max{ max }, m_onEvent{ onEvent }
	{
	}
	virtual ~SliderBase() = default;

	void setEvent(const std::function<void(T)>& onChange)
	{
		m_onEvent = onChange;
	}

protected:
	std::string_view m_label;

	std::function<void(T)> m_onEvent{};
	T m_min{ static_cast<T>(0) };
	T m_max{ static_cast<T>(1) };
	T m_value{};
	T m_valueCached{};

	friend class ImGuiUi;

	void onTick()
	{
		if (m_value == m_valueCached) return;
		m_valueCached = m_value;
		if (m_onEvent) m_onEvent(m_value);
	}

};

class SliderInt : public SliderBase<int>
{

public:
	SliderInt(int value, std::string_view label, int min, int max, const std::function<void(int)>& onEvent)
		: SliderBase(value, label, min, max, onEvent)
	{
	}

	void render() override
	{
		int i{};
		ImGui::SliderInt(m_label.data(), &m_value, m_min, m_max);
		onTick();
	}
};

class SliderFloat : public SliderBase<float>
{

public:
	SliderFloat(float value, std::string_view label, const float& min, const float& max, const std::function<void(float)>& onEvent)
		: SliderBase(value, label, min, max, onEvent)
	{
	}

	void render() override
	{
		float i{};
		ImGui::SliderFloat(m_label.data(), &m_value, m_min, m_max, "%.3f");
		onTick();
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
