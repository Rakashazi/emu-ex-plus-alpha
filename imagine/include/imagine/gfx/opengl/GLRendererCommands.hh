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
class Program;
class Renderer;
class RendererTask;

class GLRendererCommands
{
public:
	constexpr GLRendererCommands() = default;
	GLRendererCommands(RendererTask &rTask, Window *winPtr, Drawable drawable, Rect2<int> viewport,
		GLDisplay glDpy, const GLContext &glCtx, std::binary_semaphore *drawCompleteSemPtr);
	void discardTemporaryData();
	void bindGLArrayBuffer(GLuint vbo);
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	void glcMatrixMode(GLenum mode);
	#endif
	void glcBindTexture(GLenum target, GLuint texture);
	void glcDeleteTextures(GLsizei n, const GLuint *textures);
	void glcBlendFunc(GLenum sfactor, GLenum dfactor);
	void glcBlendEquation(GLenum mode);
	void glcEnable(GLenum cap);
	void glcDisable(GLenum cap);
	GLboolean glcIsEnabled(GLenum cap);
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	void glcEnableClientState(GLenum cap);
	void glcDisableClientState(GLenum cap);
	void glcTexEnvi(GLenum target, GLenum pname, GLint param);
	void glcColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
	void glcTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
	void glcColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
	void glcVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
	#endif
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	void glcVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer);
	#endif
	void setupVertexArrayPointers(const char *v, int numV, int stride,
		int textureOffset, int colorOffset, int posOffset, bool hasTexture, bool hasColor);
	void setupShaderVertexArrayPointers(const char *v, int numV, int stride, int id,
		int textureOffset, int colorOffset, int posOffset, bool hasTexture, bool hasColor);

protected:
	bool setCurrentDrawable(Drawable win);
	void setViewport(Rect2<int> v);
	void present(Drawable win);
	void doPresent();
	void notifyDrawComplete();
	void notifyPresentComplete();
	const GLContext &glContext() const;

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
	int currentVtxArrayPointerID = 0;
	#endif
	GLStateCache glState{};
	Color vColor{}; // color when using shader pipeline
	GLuint arrayBuffer = 0;
	bool arrayBufferIsSet = false;
};

using RendererCommandsImpl = GLRendererCommands;

}
