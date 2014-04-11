#pragma once

#include <imagine/engine-globals.h>
#include <imagine/gfx/Gfx.hh>
#include <imagine/util/Mem2D.hh>

namespace Gfx
{

class GeomQuadMesh
{
public:
	uint verts = 0;
	Mem2D<ColVertex> v;
	uint idxs = 0;
	VertexIndex *i = nullptr;

	constexpr GeomQuadMesh() {}
	CallResult init(const VertexPos *x, uint xVals, const VertexPos *y, uint yVals, VertexColor color = 0);
	template <size_t S1, size_t S2>
	CallResult init(const VertexPos (&x)[S1], const VertexPos (&y)[S2], VertexColor color = 0)
	{
		return init(x, S1, y, S2, color);
	}
	template <size_t S1>
	CallResult init(const VertexPos (&x)[S1], const VertexPos *y, uint yVals, VertexColor color = 0)
	{
		return init(x, S1, y, yVals, color);
	}
	template <size_t S2>
	CallResult init(const VertexPos *x, uint xVals, const VertexPos (&y)[S2], VertexColor color = 0)
	{
		return init(x, xVals, y, S2, color);
	}
	void deinit();
	void draw();
	void setColorRGB(ColorComp r, ColorComp g, ColorComp b);
	void setColorTranslucent(ColorComp a);
	void setColorRGBV(ColorComp r, ColorComp g, ColorComp b, uint i);
	void setColorTranslucentV(ColorComp a, uint i);
	void setPos(GC x, GC y, GC x2, GC y2);
} ;

}
