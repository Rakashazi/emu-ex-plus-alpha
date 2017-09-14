#pragma once

#include <cstddef>
#include <memory>
#include <imagine/config/defs.hh>
#include <imagine/gfx/Gfx.hh>
#include <imagine/util/Mem2D.hh>

namespace Gfx
{

class GeomQuadMesh
{
public:
	GeomQuadMesh() {}
	GeomQuadMesh(const VertexPos *x, uint xVals, const VertexPos *y, uint yVals, VertexColor color = 0);
	template <size_t S1, size_t S2>
	GeomQuadMesh(const VertexPos (&x)[S1], const VertexPos (&y)[S2], VertexColor color = 0):
		GeomQuadMesh(x, S1, y, S2, color) {}
	template <size_t S1>
	GeomQuadMesh(const VertexPos (&x)[S1], const VertexPos *y, uint yVals, VertexColor color = 0):
		GeomQuadMesh(x, S1, y, yVals, color) {}
	template <size_t S2>
	GeomQuadMesh(const VertexPos *x, uint xVals, const VertexPos (&y)[S2], VertexColor color = 0):
		GeomQuadMesh(x, xVals, y, S2, color) {}
	void draw(Renderer &r);
	void setColorRGB(ColorComp r, ColorComp g, ColorComp b);
	void setColorTranslucent(ColorComp a);
	void setColorRGBV(ColorComp r, ColorComp g, ColorComp b, uint i);
	void setColorTranslucentV(ColorComp a, uint i);
	void setPos(GC x, GC y, GC x2, GC y2);
	Mem2D<ColVertex> v() const;

protected:
	uint verts = 0;
	uint xVals = 0;
	std::unique_ptr<char[]> vMem{};
	uint idxs = 0;
	VertexIndex *i{};
};

}
