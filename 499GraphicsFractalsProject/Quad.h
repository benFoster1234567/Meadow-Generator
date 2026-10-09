#pragma once
#include <GL/glew.h>
#include <utility>

class Quad
{
private:
	GLuint m_vao{};
	GLuint m_vbo{};

public:
	Quad()
	{
	}

	Quad(const Quad&) = delete;
	Quad& operator=(const Quad&) = delete;

	Quad(Quad&& other) noexcept
		: m_vao(std::exchange(other.m_vao, 0)),
		m_vbo(std::exchange(other.m_vbo, 0)) {
	}

	Quad& operator=(Quad&& other) noexcept
	{
		if (this != &other) {
			destroy();
			m_vao = std::exchange(other.m_vao, 0);
			m_vbo = std::exchange(other.m_vbo, 0);
		}
		return *this;
	}

	void destroy()
	{
		if (m_vbo) { glDeleteBuffers(1, &m_vbo); m_vbo = 0; }
		if (m_vao) { glDeleteVertexArrays(1, &m_vao); m_vao = 0; }
	}
	void create()
	{
		float vertices[] = {
			-0.5f,  0.5f,    0.0f, 1.0f,
			-0.5f, -0.5f,    0.0f, 0.0f,
				0.5f, -0.5f,    1.0f, 0.0f,

			-0.5f,  0.5f,    0.0f, 1.0f,
				0.5f, -0.5f,    1.0f, 0.0f,
				0.5f,  0.5f,    1.0f, 1.0f
		};

		glGenVertexArrays(1, &m_vao);
		glGenBuffers(1, &m_vbo);

		glBindVertexArray(m_vao);
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
		glEnableVertexAttribArray(1);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	void render(GLuint shaderProgram) {
		glUseProgram(shaderProgram);
		glBindVertexArray(m_vao);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
	}

};


