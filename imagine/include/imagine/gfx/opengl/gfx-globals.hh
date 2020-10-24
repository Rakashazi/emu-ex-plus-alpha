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

namespace Gfx
{
class Renderer;
class RendererCommands;

using TransformCoordinate = GLfloat;
using VertexPos = GLfloat;
using Angle = GLfloat;
using TextureCoordinate = GLfloat;
using ColorComp = GLfloat;

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

static constexpr auto VertexColorPixelFormat = IG::PIXEL_DESC_ABGR8888;

class VertexInfo
{
public:
	static constexpr uint32_t posOffset = 0;
	static constexpr bool hasColor = false;
	static constexpr uint32_t colorOffset = 0;
	static constexpr bool hasTexture = false;
	static constexpr uint32_t textureOffset = 0;
	template<class Vtx>
	static void bindAttribs(RendererCommands &cmds, const Vtx *v);
};

class Vertex : public VertexInfo
{
public:
	VertexPos x{}, y{};

	constexpr Vertex() {};
	constexpr Vertex(VertexPos x, VertexPos y):
		x{x}, y{y} {}
	static constexpr uint32_t ID = 1;
};

class ColVertex : public VertexInfo
{
public:
	VertexPos x{}, y{};
	VertexColor color{};

	constexpr ColVertex() {};
	constexpr ColVertex(VertexPos x, VertexPos y, uint32_t color = 0):
		x{x}, y{y}, color(color) {}
	static constexpr bool hasColor = true;
	static const uint32_t colorOffset;
	static constexpr uint32_t ID = 2;
};

class TexVertex : public VertexInfo
{
public:
	VertexPos x{}, y{};
	TextureCoordinate u{}, v{};

	constexpr TexVertex() {};
	constexpr TexVertex(VertexPos x, VertexPos y, TextureCoordinate u = 0, TextureCoordinate v = 0):
		x{x}, y{y}, u{u}, v{v} {}
	static constexpr bool hasTexture = true;
	static const uint32_t textureOffset;
	static constexpr uint32_t ID = 3;
};

class ColTexVertex : public VertexInfo
{
public:
	VertexPos x{}, y{};
	TextureCoordinate u{}, v{};
	VertexColor color{};

	constexpr ColTexVertex() {};
	constexpr ColTexVertex(VertexPos x, VertexPos y, uint32_t color = 0, TextureCoordinate u = 0, TextureCoordinate v = 0):
		x{x}, y{y}, u{u}, v{v}, color(color) {}
	static constexpr bool hasColor = true;
	static const uint32_t colorOffset;
	static constexpr bool hasTexture = true;
	static const uint32_t textureOffset;
	static constexpr uint32_t ID = 4;
};

class ClipRect
{
public:
	IG::WindowRect rect{};

	constexpr ClipRect() {};
	constexpr ClipRect(int x, int y, int w, int h): rect{x, y, w, h} {}
};

using Drawable = Base::GLDrawable;

}
