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

namespace IG::Gfx
{

class RendererCommands;

static constexpr auto VertexColorPixelFormat = PIXEL_DESC_RGBA8888_NATIVE;

struct VertexInfo
{
	static constexpr uint32_t posOffset = 0;
	static constexpr bool hasColor = false;
	static constexpr uint32_t colorOffset = 0;
	static constexpr bool hasTexture = false;
	static constexpr uint32_t textureOffset = 0;
	template<class Vtx>
	static void bindAttribs(RendererCommands &cmds, const Vtx *v);
};

template <class T>
concept Vertex = IG::derived_from<T, VertexInfo>;

struct Vertex2D : public VertexInfo
{
	VertexPos x{}, y{};
	static constexpr uint32_t ID = 1;
};

struct ColVertex : public VertexInfo
{
	VertexPos x{}, y{};
	VertexColor color{};
	static constexpr bool hasColor = true;
	static const uint32_t colorOffset;
	static constexpr uint32_t ID = 2;
};

struct TexVertex : public VertexInfo
{
	VertexPos x{}, y{};
	float u{}, v{};
	static constexpr bool hasTexture = true;
	static const uint32_t textureOffset;
	static constexpr uint32_t ID = 3;
};

struct ColTexVertex : public VertexInfo
{
	VertexPos x{}, y{};
	float u{}, v{};
	VertexColor color{};
	static constexpr bool hasColor = true;
	static const uint32_t colorOffset;
	static constexpr bool hasTexture = true;
	static const uint32_t textureOffset;
	static constexpr uint32_t ID = 4;
};

}
