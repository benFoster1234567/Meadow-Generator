#pragma once
#include <GL/glew.h>
#include <IL/il.h>
#include "Light.h"
#include "GLDebugHandler.h"
#include <glm/ext/matrix_float4x4.hpp>
class ShadowFBO
{
public:

	void bind() const
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_shadowMapFBO);
	}

	void clearBuffer()
	{
		glClear(GL_DEPTH_BUFFER_BIT);
	}

	void setViewport() const
	{
		glViewport(0, 0, m_width, m_height);
	}

	void enableCullFace()
	{
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);
	}

	void postShadowPass()
	{
		glFlush();
	}

	void setUpFBO()
	{
		GLfloat border[] = { 1.0f, 0.0f, 0.0f, 0.0f };
		glGenTextures(1, &m_depthMapTexture);
		glBindTexture(GL_TEXTURE_2D, m_depthMapTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, m_width, m_height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LESS);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_depthMapTexture);

		glGenFramebuffers(1, &m_shadowMapFBO);
		glBindFramebuffer(GL_FRAMEBUFFER, m_shadowMapFBO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthMapTexture, 0);

		glDrawBuffer(GL_NONE);
		GLenum result = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (result == GL_FRAMEBUFFER_COMPLETE) {
			DBG_LOG("Framebuffer is complete.\n");
		}
		else {
			DBG_LOG("Framebuffer is not complete.\n");
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void exportDepthBuffer() const
	{
		//from gpu gems
		int size = m_width * m_height;
		float* buffer = new float[size];
		unsigned char* imgBuffer = new unsigned char[size * 4];

		glGetTexImage(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, GL_FLOAT, buffer);

		for (int i = 0; i < m_height; i++)
			for (int j = 0; j < m_width; j++)
			{
				int imgIdx = 4 * ((i * m_width) + j);
				int bufIdx = ((m_height - i - 1) * m_width) + j;

				// This is just to make a more visible image.  Scale so that
				// the range (minVal, 1.0) maps to (0.0, 1.0).  This probably should
				// be tweaked for different light configurations.
				float minVal = 0.88f;
				float scale = (buffer[bufIdx] - minVal) / (1.0f - minVal);
				unsigned char val = (unsigned char)(scale * 255);
				imgBuffer[imgIdx] = val;
				imgBuffer[imgIdx + 1] = val;
				imgBuffer[imgIdx + 2] = val;
				imgBuffer[imgIdx + 3] = 0xff;
			}
		unsigned int imageID;

		// Load Texture Map
		ilGenImages(1, &imageID);

		ilBindImage(imageID); /* Binding of DevIL image name */
		ilEnable(IL_ORIGIN_SET);
		ilOriginFunc(IL_ORIGIN_LOWER_LEFT);
		ilTexImage(m_width, m_height, 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, imgBuffer /*Texture*/);
		ilSave(IL_PNG, (wchar_t*)("output.png"));

		delete[] buffer;
		delete[] imgBuffer;
	}

private:
	GLuint m_shadowMapFBO{ 0 };
	GLuint m_depthMapTexture{ 0 };
	const unsigned int m_width = 1000;
	const unsigned int m_height = 1000;
};