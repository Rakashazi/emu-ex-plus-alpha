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
#include <imagine/gfx/defs.hh>
#include "defs.hh"
#include <imagine/util/memory/UniqueResource.hh>
#include <utility>

namespace IG::Gfx
{

class RendererTask;
class RendererCommands;
class Text;
class Mat4;

// Shader

using NativeShader = GLuint;

void destroyGLShader(RendererTask &, NativeShader);

struct GLShaderDeleter
{
	RendererTask *rTask{};

	void operator()(NativeShader s) const
	{
		destroyGLShader(*rTask, s);
	}
};
using UniqueGLShader = UniqueResource<NativeShader, GLShaderDeleter>;

using ShaderImpl = UniqueGLShader;

// Program

using NativeProgram = GLuint;

void destroyGLProgram(RendererTask &, NativeProgram);

struct GLProgramDeleter
{
	RendererTask *rTask{};

	void operator()(NativeProgram p) const
	{
		destroyGLProgram(*rTask, p);
	}
};
using UniqueGLProgram = UniqueResource<NativeProgram, GLProgramDeleter>;

class GLSLProgram
{
public:
	constexpr GLSLProgram() = default;
	constexpr NativeProgram glProgram() const { return program_; }
	constexpr bool operator ==(GLSLProgram const &rhs) const { return program_.get() == rhs.program_.get(); }
	constexpr NativeProgram release() { return program_.release(); }
	constexpr RendererTask &task() { return *program_.get_deleter().rTask; }

protected:
	UniqueGLProgram program_{};
};

using ProgramImpl = GLSLProgram;

}
