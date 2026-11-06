#pragma once
#include <GL/glew.h>
#include <iostream>
#include <string>

/// <summary>
/// For screenspace to world space object selection
/// </summary>
class PickingTexture
{
public:
	PickingTexture() {}
	~PickingTexture() = default;

	void initialize(const unsigned int& width, const unsigned int& height)
	{
		if (m_initialized) return;

		glGenFramebuffers(1, &m_fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

		glGenTextures(1, &m_pickingTexture);
		glBindTexture(GL_TEXTURE_2D, m_pickingTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, width, height, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_pickingTexture, 0);

		glGenTextures(1, &m_depthTexture);
		glBindTexture(GL_TEXTURE_2D, m_depthTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_COMPONENT, GL_TEXTURE_2D, m_depthTexture, 0);

		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE)
		{
			throw std::runtime_error(std::format("Framebuffer error, status: 0x{:x}", status));
		}

		glBindTexture(GL_TEXTURE_2D, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		m_initialized = true;
	}

	void resize(const unsigned int& width, const unsigned int& height)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
		glBindTexture(GL_TEXTURE_2D, m_pickingTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, width, height, 0, GL_RGB_INTEGER, GL_UNSIGNED_INT, NULL);
		glBindTexture(GL_TEXTURE_2D, 0);

		glBindTexture(GL_TEXTURE_2D, m_depthTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE)
		{
			throw std::runtime_error(std::format("Framebuffer error, status: 0x{:x}", status));
		}

		glBindTexture(GL_TEXTURE_2D, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void enableWriting() const
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	}

	void disableWriting() const
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	struct PixelInfo
	{
		unsigned int objectID{};

		void print() const
		{
			std::cout << "Object " << objectID << "\n";
		}
	};

	PixelInfo readPixel(int x, int y)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
		glReadBuffer(GL_COLOR_ATTACHMENT0);
		PixelInfo pixel;
		glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_UNSIGNED_INT, &pixel);
		glReadBuffer(GL_NONE);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return pixel;
	}

private:
	GLuint m_fbo{};
	GLuint m_pickingTexture{};
	GLuint m_depthTexture{};
	bool m_initialized{ false };
};