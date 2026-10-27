#pragma once

#include <GL/glew.h>
#include <vector>


class GlObjectBase
{
protected:

	using BufferId = size_t;

	inline static BufferId s_boundBuffer = 0;
	inline static BufferId s_nextBufferId = 1;
	inline static std::vector<bool> boundObjects{ 100, false };
	BufferId m_id;

	GLuint gl_bufferObject{ 0 };

public:
	GlObjectBase()
	{
		m_id = s_nextBufferId;
		s_nextBufferId++;
	}

	bool isBound() const
	{
		return boundObjects[m_id];
	}

	virtual void generate() = 0;
	virtual void bind() = 0;
	virtual void unbind() = 0;

};

