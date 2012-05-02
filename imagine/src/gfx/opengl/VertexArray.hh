#pragma once

#include <gfx/VertexArray.hh>

void VertexArray::init(const void *data, size_t size)
{
	if(useVBOFuncs)
	{
		glGenBuffers(1, &ref);
		logMsg("new VBO id %d", ref);
		if(data)
			write(data, size);
	}
}

void VertexArray::write(const void *data, size_t size)
{
	if(useVBOFuncs)
	{
		glState_bindBuffer(GL_ARRAY_BUFFER, ref);
		//glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
		glBufferData(GL_ARRAY_BUFFER, size, data, GL_DYNAMIC_DRAW);
	}
}

void VertexArray::deinit()
{
	if(useVBOFuncs)
	{
		logMsg("deleting VBO id %d", ref);
		glDeleteBuffers(1, &ref);
	}
}
