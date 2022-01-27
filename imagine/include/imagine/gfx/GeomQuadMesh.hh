#pragma once

#include <cstddef>
#include <memory>
#include <imagine/config/defs.hh>
#include <imagine/gfx/defs.hh>
#include <imagine/gfx/Vertex.hh>
#include <imagine/util/container/array.hh>

namespace IG::Gfx
{

class RendererCommands;

class GeomQuadMesh
{
public:
	constexpr GeomQuadMesh() = default;
	GeomQuadMesh(const VertexPos *x, uint32_t xVals, const VertexPos *y, uint32_t yVals, VertexColor color = 0);
	template <size_t S1, size_t S2>
	GeomQuadMesh(const VertexPos (&x)[S1], const VertexPos (&y)[S2], VertexColor color = 0):
		GeomQuadMesh(x, S1, y, S2, color) {}
	template <size_t S1>
	GeomQuadMesh(const VertexPos (&x)[S1], const VertexPos *y, uint32_t yVals, VertexColor color = 0):
		GeomQuadMesh(x, S1, y, yVals, color) {}
	template <size_t S2>
	GeomQuadMesh(const VertexPos *x, uint32_t xVals, const VertexPos (&y)[S2], VertexColor color = 0):
		GeomQuadMesh(x, xVals, y, S2, color) {}
	void draw(RendererCommands &r) const;
	void setColorRGB(ColorComp r, ColorComp g, ColorComp b);
	void setColorTranslucent(ColorComp a);
	void setColorRGBV(ColorComp r, ColorComp g, ColorComp b, uint32_t i);
	void setColorTranslucentV(ColorComp a, uint32_t i);
	void setPos(float x, float y, float x2, float y2);
	IG::ArrayView2<ColVertex> v() const;

protected:
	std::unique_ptr<char[]> vMem{};
	VertexIndex *i{};
	uint32_t verts = 0;
	uint32_t idxs = 0;
	uint32_t xVals = 0;
};

}
