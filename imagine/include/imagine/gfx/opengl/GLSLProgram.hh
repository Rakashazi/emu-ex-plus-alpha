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

class RendererTask;
class RendererCommands;

using Shader = GLuint;
#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
enum { SHADER_VERTEX = GL_VERTEX_SHADER, SHADER_FRAGMENT = GL_FRAGMENT_SHADER };
#else
enum { SHADER_VERTEX = 1, SHADER_FRAGMENT = 2 }; // dummy values
#endif

class GLSLProgram
{
public:
	constexpr GLSLProgram() {}
	bool init(RendererTask &, Shader vShader, Shader fShader, bool hasColor, bool hasTex);
	void deinit(RendererTask &);
	bool link(RendererTask &);
	GLint modelViewProjectionUniform() const;
	GLuint program() const { return program_; }
	explicit operator bool() const;

protected:
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	GLuint program_ = 0;
	GLint mvpUniform = -1;

	void initUniforms(RendererTask &);
	#endif
};

template <bool hasColor, bool hasTex>
class DefaultProgram : public GLSLProgram
{
public:
	constexpr DefaultProgram() {}

	void init(RendererTask &rTask, GLuint vShader, GLuint fShader)
	{
		GLSLProgram::init(rTask, vShader, fShader, hasColor, hasTex);
		link(rTask);
	}
};

using TexProgram = DefaultProgram<true, true>;
using ColorProgram = DefaultProgram<true, false>;

// default programs

class DefaultTexReplaceProgram : public TexProgram
{
public:
	constexpr DefaultTexReplaceProgram() {}
	bool compile(RendererTask &);
	void use(RendererCommands &cmds) const { use(cmds, nullptr); }
	void use(RendererCommands &cmds, Mat4 modelMat) const { use(cmds, &modelMat); }
	void use(RendererCommands &cmds, const Mat4 *modelMat) const;
};

class DefaultTexProgram : public TexProgram
{
public:
	constexpr DefaultTexProgram() {}
	bool compile(RendererTask &);
	void use(RendererCommands &cmds) const { use(cmds, nullptr); }
	void use(RendererCommands &cmds, Mat4 modelMat) const { use(cmds, &modelMat); }
	void use(RendererCommands &cmds, const Mat4 *modelMat) const;
};

class DefaultTexAlphaReplaceProgram : public TexProgram
{
public:
	DefaultTexProgram *impl{};
	constexpr DefaultTexAlphaReplaceProgram() {}
	bool compile(RendererTask &);
	void use(RendererCommands &cmds) const { use(cmds, nullptr); }
	void use(RendererCommands &cmds, Mat4 modelMat) const { use(cmds, &modelMat); }
	void use(RendererCommands &cmds, const Mat4 *modelMat) const;
};

class DefaultTexAlphaProgram : public TexProgram
{
public:
	DefaultTexProgram *impl{};
	constexpr DefaultTexAlphaProgram() {}
	bool compile(RendererTask &);
	void use(RendererCommands &cmds) const { use(cmds, nullptr); }
	void use(RendererCommands &cmds, Mat4 modelMat) const { use(cmds, &modelMat); }
	void use(RendererCommands &cmds, const Mat4 *modelMat) const;
};

class DefaultTexExternalReplaceProgram : public TexProgram
{
public:
	constexpr DefaultTexExternalReplaceProgram() {}
	bool compile(RendererTask &);
	void use(RendererCommands &cmds) const { use(cmds, nullptr); }
	void use(RendererCommands &cmds, Mat4 modelMat) const { use(cmds, &modelMat); }
	void use(RendererCommands &cmds, const Mat4 *modelMat) const;
};

class DefaultTexExternalProgram : public TexProgram
{
public:
	constexpr DefaultTexExternalProgram() {}
	bool compile(RendererTask &);
	void use(RendererCommands &cmds) const { use(cmds, nullptr); }
	void use(RendererCommands &cmds, Mat4 modelMat) const { use(cmds, &modelMat); }
	void use(RendererCommands &cmds, const Mat4 *modelMat) const;
};

class DefaultColorProgram : public ColorProgram
{
public:
	constexpr DefaultColorProgram() {}
	bool compile(RendererTask &);
	void use(RendererCommands &cmds) const { use(cmds, nullptr); }
	void use(RendererCommands &cmds, Mat4 modelMat) const { use(cmds, &modelMat); }
	void use(RendererCommands &cmds, const Mat4 *modelMat) const;
};

using ProgramImpl = GLSLProgram;

}
