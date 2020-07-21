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
#include <imagine/gfx/Viewport.hh>

namespace Gfx
{

class TextureSampler;
class GLSLProgram;

class GLRendererCommands
{
public:
	const TextureSampler *currSampler{};
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	uint32_t currentVtxArrayPointerID = 0;
	Mat4 modelMat, projectionMat;
	#endif
	GLStateCache glState{};

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
	void glcTexEnvfv(GLenum target, GLenum pname, const GLfloat *params);
	void glcColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
	void glcTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
	void glcColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
	void glcVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
	#endif
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	void glcVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer);
	#endif

protected:
	Viewport currViewport;
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	GLSLProgram *currProgram{};
	#endif
	std::array<ColorComp, 4> vColor{}; // color when using shader pipeline
	std::array<ColorComp, 4> texEnvColor{}; // color when using shader pipeline
	GLuint arrayBuffer = 0;
	bool arrayBufferIsSet = false;
};

using RendererCommandsImpl = GLRendererCommands;

}
