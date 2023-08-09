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
#include <imagine/gfx/Vertex.hh>
#include <imagine/util/container/array.hh>
#include <cstddef>
#include <memory>
#include <span>

namespace IG::Gfx
{

class RendererCommands;

class GeomQuadMesh
{
public:
	using Vertex = Vertex2IColI;

	constexpr GeomQuadMesh() = default;
	GeomQuadMesh(std::span<const int> x, std::span<const int> y, PackedColor color = {});
	void draw(RendererCommands &r) const;
	void setColor(PackedColor);
	void setColorV(PackedColor, size_t i);
	void setPos(int x, int y, int x2, int y2);
	ArrayView2<Vertex> v() const;

protected:
	std::unique_ptr<char[]> vMem;
	VertexIndex *i{};
	int verts{};
	int idxs{};
	int xVals{};
};

}
