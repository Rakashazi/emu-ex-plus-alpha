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
#include <imagine/gfx/Mat4.hh>
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
	void bindGLArrayBuffer(GLuint vbo);
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	void glcMatrixMode(GLenum mode);
	#endif
	void glcBlendFunc(GLenum sfactor, GLenum dfactor);
	void glcBlendEquation(GLenum mode);
	void glcEnable(GLenum cap);
	void glcDisable(GLenum cap);
	GLboolean glcIsEnabled(GLenum cap);
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	void glcEnableClientState(GLenum cap);
	void glcDisableClientState(GLenum cap);
	void glcColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
	#endif
	void setupVertexArrayPointers(const char *v, int stride,
		AttribDesc textureAttrib, AttribDesc colorAttrib, AttribDesc posAttrib);
	void setupShaderVertexArrayPointers(const char *v, int stride, VertexLayoutFlags enabledLayout,
		AttribDesc textureAttrib, AttribDesc colorAttrib, AttribDesc posAttrib);

protected:
	bool setCurrentDrawable(Drawable win);
	void setViewport(Rect2<int> v);
	void present(Drawable win);
	void doPresent();
	void notifyDrawComplete();
	void notifyPresentComplete();
	const GLContext &glContext() const;
	bool hasVBOFuncs() const;
	bool useFixedFunctionPipeline() const;

	template<VertexLayout V>
	void setupVertexArrayPointers(const V *v)
	{
		setupVertexArrayPointers((const char*)v, sizeof(V),
			texCoordAttribDesc<V>(), colorAttribDesc<V>(), posAttribDesc<V>());
	}

	template<VertexLayout V>
	void setupShaderVertexArrayPointers(const V *v)
	{
		setupShaderVertexArrayPointers((const char*)v, sizeof(V), vertexLayoutEnableMask<V>(),
			texCoordAttribDesc<V>(), colorAttribDesc<V>(), posAttribDesc<V>());
	}

	void setVertexAttribs(VertexLayout auto *v)
	{
		if(hasVBOFuncs())
			v = nullptr;
		#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
		if(useFixedFunctionPipeline())
		{
			setupVertexArrayPointers(v);
			return;
		}
		#endif
		#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
		setupShaderVertexArrayPointers(v);
		#endif
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
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	NativeProgram currProgram{};
	VertexLayoutFlags currentEnabledVertexLayout{};
	#endif
	GLStateCache glState{};
	Color4F vColor{}; // color when using shader pipeline
};

using RendererCommandsImpl = GLRendererCommands;

}
