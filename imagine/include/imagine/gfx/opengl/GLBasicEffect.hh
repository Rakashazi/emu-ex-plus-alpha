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
#include "GLSLProgram.hh"
#include <span>
#include <string_view>

namespace IG::Gfx
{

class RendererCommands;
class RendererTask;
class Mat4;

class GLBasicEffect
{
public:
	bool setShaders(RendererTask &, std::span<std::string_view> vertSrcs, std::span<std::string_view> fragSrcs);

protected:
	NativeProgram program{};
	GLint modelViewUniform = -1;
	GLint projUniform = -1;
	GLint textureModeUniform = -1;
	TextureType texType{};

	void updateTextureType(RendererCommands &, TextureType);
	void setModelView(RendererCommands &, Mat4);
	void setProjection(RendererCommands &, Mat4);
};

using BasicEffectImpl = GLBasicEffect;

}
