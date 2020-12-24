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

class GLSLProgram
{
public:
	constexpr GLSLProgram() {}
	GLint modelViewProjectionUniform() const;
	GLuint program() const { return program_; }

protected:
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	GLuint program_ = 0;
	GLint mvpUniform = -1;

	void initUniforms(RendererTask &);
	#endif
};

using ProgramImpl = GLSLProgram;

}
