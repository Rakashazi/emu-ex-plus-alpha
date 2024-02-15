#pragma once

/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/config/defs.hh>
#include <imagine/gfx/opengl/GLStateCache.hh>
#include <imagine/gfx/Vertex.hh>
#include <imagine/thread/Semaphore.hh>
#include "GLSLProgram.hh"
#include <imagine/util/used.hh>

namespace IG
{
class GLContext;
}

namespace IG::Gfx
{

class TextureSampler;

class GLRendererCommands
{
public:
	constexpr GLRendererCommands() = default;
	GLRendererCommands(RendererTask &rTask, Window *winPtr, Drawable drawable, Rect2<int> viewport,
		GLDisplay glDpy, const GLContext &glCtx, std::binary_semaphore *drawCompleteSemPtr);
	void bindGLVertexArray(GLuint vao);
	void bindGLArrayBuffer(GLuint vbo);
	void bindGLIndexBuffer(GLuint ibo);
	void glcBlendFunc(GLenum sfactor, GLenum dfactor);
	void glcBlendEquation(GLenum mode);
	void glcEnable(GLenum cap);
	void glcDisable(GLenum cap);
	GLboolean glcIsEnabled(GLenum cap);
	void setupVertexArrayPointers(int stride,
		AttribDesc textureAttrib, AttribDesc colorAttrib, AttribDesc posAttrib);
	void setupVertexArrayPointers(int stride, VertexLayoutFlags enabledLayout, VertexLayoutDesc);

protected:
	bool setCurrentDrawable(Drawable win);
	void setViewport(Rect2<int> v);
	void present(Drawable win);
	void doPresent();
	void notifyDrawComplete();
	void notifyPresentComplete();
	const GLContext &glContext() const;
	bool hasVAOFuncs() const;

	template<VertexLayout V>
	void setupVertexArrayPointers()
	{
		setupVertexArrayPointers(sizeof(V),
			texCoordAttribDesc<V>(), colorAttribDesc<V>(), posAttribDesc<V>());
	}

	template<VertexLayout V>
	void setVertexAttribs()
	{
		setupVertexArrayPointers(sizeof(V), vertexLayoutEnableMask<V>(), vertexLayoutDesc<V>());
	}

	template<class V>
	void setVertexArray(const ObjectVertexArray<V> &verts)
	{
		if(hasVAOFuncs())
		{
			bindGLVertexArray(verts.array().name());
		}
		else
		{
			bindGLArrayBuffer(verts.name());
			bindGLIndexBuffer(verts.array().name()); // index buffer name is stored in place of array name
			setVertexAttribs<typename ObjectVertexArray<V>::Vertex>();
		}
	}

	template<class V>
	void setVertexBuffer(const Buffer<V, BufferType::vertex> &verts)
	{
		bindGLArrayBuffer(verts.name());
	}

	RendererTask *rTask{};
	Renderer *r{};
	std::binary_semaphore *drawCompleteSemPtr{};
	Window *winPtr{};
	[[no_unique_address]] GLDisplay glDpy{};
	const GLContext *glContextPtr{};
	Drawable drawable{};
	Rect2<int> winViewport{};
	GLuint currSamplerName{};
	GLuint currVertexArrayName{};
	GLuint currIndexBufferName{};
	NativeProgram currProgram{};
	VertexLayoutFlags currentEnabledVertexLayout{};
	GLStateCache glState{};
	Color4F vColor{}; // color when using shader pipeline
};

using RendererCommandsImpl = GLRendererCommands;

}
