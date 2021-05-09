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
#include <imagine/base/GLContext.hh>
#include <array>

namespace Gfx
{

using TransformCoordinate = GLfloat;
using VertexPos = GLfloat;
using Angle = GLfloat;
using TextureCoordinate = GLfloat;
using ColorComp = GLfloat;
using Color = std::array<GLfloat, 4>;

static constexpr Angle angleFromDegree(Angle deg) { return IG::radians(deg); }
static constexpr Angle angleFromRadian(Angle rad) { return rad; }
static constexpr Angle angleToDegree(Angle a) { return IG::degrees(a); }
static constexpr Angle angleToRadian(Angle a) { return a; }

using TextureRef = GLuint;
using VertexIndex = GLushort;
using VertexColor = uint32_t;
using VertexArrayRef = uint32_t;

static constexpr int TRIANGLE_IMPL = GL_TRIANGLES;
static constexpr int TRIANGLE_STRIP_IMPL = GL_TRIANGLE_STRIP;

static constexpr int ZERO_IMPL = GL_ZERO;
static constexpr int ONE_IMPL = GL_ONE;
static constexpr int SRC_COLOR_IMPL = GL_SRC_COLOR;
static constexpr int ONE_MINUS_SRC_COLOR_IMPL = GL_ONE_MINUS_SRC_COLOR;
static constexpr int DST_COLOR_IMPL = GL_DST_COLOR;
static constexpr int ONE_MINUS_DST_COLOR_IMPL = GL_ONE_MINUS_DST_COLOR;
static constexpr int SRC_ALPHA_IMPL = GL_SRC_ALPHA;
static constexpr int ONE_MINUS_SRC_ALPHA_IMPL = GL_ONE_MINUS_SRC_ALPHA;
static constexpr int DST_ALPHA_IMPL = GL_DST_ALPHA;
static constexpr int ONE_MINUS_DST_ALPHA_IMPL = GL_ONE_MINUS_DST_ALPHA;
static constexpr int CONSTANT_COLOR_IMPL = GL_CONSTANT_COLOR;
static constexpr int ONE_MINUS_CONSTANT_COLOR_IMPL = GL_ONE_MINUS_CONSTANT_COLOR;
static constexpr int CONSTANT_ALPHA_IMPL = GL_CONSTANT_ALPHA;
static constexpr int ONE_MINUS_CONSTANT_ALPHA_IMPL = GL_ONE_MINUS_CONSTANT_ALPHA;

static constexpr int SYNC_FLUSH_COMMANDS_BIT = GL_SYNC_FLUSH_COMMANDS_BIT;

using ClipRect = IG::WindowRect;
using Drawable = Base::NativeGLDrawable;

enum class ShaderType : uint16_t
{
	VERTEX = GL_VERTEX_SHADER,
	FRAGMENT = GL_FRAGMENT_SHADER
};

enum class ColorSpace : uint8_t
{
	LINEAR = (uint8_t)Base::GLColorSpace::LINEAR,
	SRGB = (uint8_t)Base::GLColorSpace::SRGB,
};

}
