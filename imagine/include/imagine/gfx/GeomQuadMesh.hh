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
	constexpr GeomQuadMesh() = default;
	GeomQuadMesh(std::span<const float> x, std::span<const float> y, VertexColor color = {});
	void draw(RendererCommands &r) const;
	void setColor(VertexColor);
	void setColorV(VertexColor, size_t i);
	void setPos(float x, float y, float x2, float y2);
	ArrayView2<Vertex2PCol> v() const;

protected:
	std::unique_ptr<char[]> vMem{};
	VertexIndex *i{};
	int verts{};
	int idxs{};
	int xVals{};
};

}
