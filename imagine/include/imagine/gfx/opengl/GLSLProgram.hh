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
#include "glIncludes.h"
#include "defs.hh"
#include <imagine/gfx/Mat4.hh>

namespace Gfx
{

class Renderer;
class RendererCommands;

using Shader = GLuint;
#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
enum { SHADER_VERTEX = GL_VERTEX_SHADER, SHADER_FRAGMENT = GL_FRAGMENT_SHADER };
#else
enum { SHADER_VERTEX = 1, SHADER_FRAGMENT = 2 }; // dummy values
#endif

class GLSLProgram
{
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
protected:
	GLuint program_ = 0;

public:
	GLint modelViewProjectionUniform = -1;

	GLuint program() { return program_; }

	void initUniforms(Renderer &r);
	#endif

public:
	constexpr GLSLProgram() {}
	bool init(Renderer &r, Shader vShader, Shader fShader, bool hasColor, bool hasTex);
	void deinit(Renderer &r);
	bool link(Renderer &r);
	explicit operator bool() const
	{
		#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
		return program_;
		#else
		return true;
		#endif
	}
};

class TexProgram : public GLSLProgram
{
public:
	constexpr TexProgram() {}
	void init(Renderer &r, GLuint vShader, GLuint fShader);
};

class ColorProgram : public GLSLProgram
{
public:
	constexpr ColorProgram() {}
	void init(Renderer &r, GLuint vShader, GLuint fShader);
};

// default programs

class DefaultTexReplaceProgram : public TexProgram
{
public:
	constexpr DefaultTexReplaceProgram() {}
	bool compile(Renderer &r);
	void use(RendererCommands &cmds) { use(cmds, nullptr); }
	void use(RendererCommands &cmds, Mat4 modelMat) { use(cmds, &modelMat); }
	void use(RendererCommands &cmds, const Mat4 *modelMat);
};

class DefaultTexProgram : public TexProgram
{
public:
	constexpr DefaultTexProgram() {}
	bool compile(Renderer &r);
	void use(RendererCommands &cmds) { use(cmds, nullptr); }
	void use(RendererCommands &cmds, Mat4 modelMat) { use(cmds, &modelMat); }
	void use(RendererCommands &cmds, const Mat4 *modelMat);
};

class DefaultTexAlphaReplaceProgram : public TexProgram
{
public:
	DefaultTexProgram *impl{};
	constexpr DefaultTexAlphaReplaceProgram() {}
	bool compile(Renderer &r);
	void use(RendererCommands &cmds) { use(cmds, nullptr); }
	void use(RendererCommands &cmds, Mat4 modelMat) { use(cmds, &modelMat); }
	void use(RendererCommands &cmds, const Mat4 *modelMat);
};

class DefaultTexAlphaProgram : public TexProgram
{
public:
	DefaultTexProgram *impl{};
	constexpr DefaultTexAlphaProgram() {}
	bool compile(Renderer &r);
	void use(RendererCommands &cmds) { use(cmds, nullptr); }
	void use(RendererCommands &cmds, Mat4 modelMat) { use(cmds, &modelMat); }
	void use(RendererCommands &cmds, const Mat4 *modelMat);
};

class DefaultTexExternalReplaceProgram : public TexProgram
{
public:
	constexpr DefaultTexExternalReplaceProgram() {}
	bool compile(Renderer &r);
	void use(RendererCommands &cmds) { use(cmds, nullptr); }
	void use(RendererCommands &cmds, Mat4 modelMat) { use(cmds, &modelMat); }
	void use(RendererCommands &cmds, const Mat4 *modelMat);
};

class DefaultTexExternalProgram : public TexProgram
{
public:
	constexpr DefaultTexExternalProgram() {}
	bool compile(Renderer &r);
	void use(RendererCommands &cmds) { use(cmds, nullptr); }
	void use(RendererCommands &cmds, Mat4 modelMat) { use(cmds, &modelMat); }
	void use(RendererCommands &cmds, const Mat4 *modelMat);
};

class DefaultColorProgram : public ColorProgram
{
public:
	constexpr DefaultColorProgram() {}
	bool compile(Renderer &r);
	void use(RendererCommands &cmds) { use(cmds, nullptr); }
	void use(RendererCommands &cmds, Mat4 modelMat) { use(cmds, &modelMat); }
	void use(RendererCommands &cmds, const Mat4 *modelMat);
};

using ProgramImpl = GLSLProgram;

}
