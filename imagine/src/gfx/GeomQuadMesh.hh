#pragma once

#include <gfx/Gfx.hh>
#include <util/Array2D.hh>

namespace Gfx
{

class GeomQuadMesh
{
public:
	constexpr GeomQuadMesh() { }
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
	void setColorRGB(GColor r, GColor g, GColor b);
	void setColorTranslucent(GColor a);
	void setColorRGBV(GColor r, GColor g, GColor b, uint i);
	void setColorTranslucentV(GColor a, uint i);
	void setPos(GC x, GC y, GC x2, GC y2);

	uint verts = 0;
	Array2D<ColVertex> v;
	uint idxs = 0;
	VertexIndex *i = nullptr;
} ;

}
