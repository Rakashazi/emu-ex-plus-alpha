#pragma once

#include <imagine/config/defs.hh>
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
