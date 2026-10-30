#pragma once
#include <iostream>
#include <sstream>
#include <array>
#include <cstdint>

#include <GL/glew.h>

#ifdef LOG_DEBUG_INFO
#define DBG_LOG(type, msg) DebugHandler::instance().log(type, msg)
#else
#define DBG_LOG(type, msg) ((void)0)
#endif

enum class DebugMsgType : uint8_t
{
	Default,
	Mesh,
	GLStatus,
	GLError,
	GLWarning,
	Count
};

class DebugHandler
{
public:

	static DebugHandler& instance()
	{
		static DebugHandler i;
		return i;
	}

	DebugHandler(const DebugHandler&) = delete;
	DebugHandler& operator=(const DebugHandler&) = delete;
	DebugHandler(DebugHandler&&) = delete;
	DebugHandler& operator=(DebugHandler&&) = delete;

	~DebugHandler()
	{
		flush(*m_out);
	}

	static void setUpOpenGL()
	{
#ifdef LOG_DEBUG_INFO
		static bool installed = false;
		if (installed) return;
		installed = true;

		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

		glDebugMessageCallback(glCallback, nullptr);
#endif
	}

	void log(DebugMsgType type, const std::string& message)
	{
		if (type >= DebugMsgType::Count)
		{
			return;
		}

		if (m_record[static_cast<size_t>(type)])
			m_buffer << message << "\n";
	}

	void log(const std::string& message)
	{
		log(DebugMsgType::Default, message);
	}

	void setTypeRecord(DebugMsgType type, bool recording)
	{
		if (type >= DebugMsgType::Count)
		{
			return;
		}

		m_record[static_cast<size_t>(type)] = recording;
	}

	void flush(std::ostream& l_out)
	{
		std::string s = m_buffer.str();
		if (s.empty()) return;
		l_out << s;
		m_buffer.str("");
		m_buffer.clear();
	}

	void flush()
	{
		flush(*m_out);
	}

	void tick(float deltaTime)
	{
		timeElapsed += deltaTime;
		if (timeElapsed >= updateIntervalS)
		{
			flush(*m_out);
			timeElapsed = 0.f;
		}
	}

	void setOutput(std::ostream& output)
	{
		m_out = &output;
	}

private:
	DebugHandler()
	{
		m_record.fill(true);
	}

	static void GLAPIENTRY glCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
		GLsizei length, const GLchar* message, const void* userParam)
	{
		if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) return;

		std::string text = "GL CALLBACK: ";
		if (type == GL_DEBUG_TYPE_ERROR)
		{
			text += "** GL ERROR ** ";
		}

		text += "type = " + std::to_string(type)
			+ ", severity = " + std::to_string(severity)
			+ ", message = " + message;

		DebugHandler::instance().log(type == GL_DEBUG_TYPE_ERROR ? DebugMsgType::GLError : DebugMsgType::GLStatus, text);
	}

	std::stringstream m_buffer;
	std::array<bool, static_cast<size_t>(DebugMsgType::Count)> m_record{};

	std::ostream* m_out{ &std::cerr };
	float updateIntervalS = { 3.f };
	float timeElapsed{ 0.f };
};