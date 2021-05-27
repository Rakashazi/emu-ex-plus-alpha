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

#include <imagine/gfx/defs.hh>
#include <imagine/pixmap/PixelDesc.hh>

namespace Gfx
{

class RendererCommands;

static constexpr auto VertexColorPixelFormat = IG::PIXEL_DESC_RGBA8888.nativeOrder();

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

}
