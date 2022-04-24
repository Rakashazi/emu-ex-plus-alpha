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
	GeomQuadMesh(std::span<const VertexPos> x, std::span<const VertexPos> y, VertexColor color = 0);
	void draw(RendererCommands &r) const;
	void setColorRGB(ColorComp r, ColorComp g, ColorComp b);
	void setColorTranslucent(ColorComp a);
	void setColorRGBV(ColorComp r, ColorComp g, ColorComp b, size_t i);
	void setColorTranslucentV(ColorComp a, size_t i);
	void setPos(float x, float y, float x2, float y2);
	IG::ArrayView2<ColVertex> v() const;

protected:
	std::unique_ptr<char[]> vMem{};
	VertexIndex *i{};
	int verts{};
	int idxs{};
	int xVals{};
};

}
