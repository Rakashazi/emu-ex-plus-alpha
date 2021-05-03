#pragma once

#include <imagine/config/defs.hh>
#include <imagine/gfx/defs.hh>
#include <imagine/gfx/Vertex.hh>
#include <imagine/util/rectangle2.h>
#include <imagine/util/edge.h>

namespace Gfx
{

class RendererCommands;
class ProjectionPlane;

template<class Vtx>
static constexpr std::array<Vtx, 4> mapQuadUV(std::array<Vtx, 4> v, GTexCRect rect)
{
	v[0].u = rect.x;  v[0].v = rect.y2; //BL
	v[1].u = rect.x;  v[1].v = rect.y;  //TL
	v[2].u = rect.x2; v[2].v = rect.y2; //BR
	v[3].u = rect.x2; v[3].v = rect.y;  //TR
	return v;
}

template<class Vtx>
static constexpr std::array<Vtx, 4> mapQuadPos(std::array<Vtx, 4> v, GP bl, GP tl, GP tr, GP br)
{
	v[0].x = bl.x; v[0].y = bl.y;
	v[1].x = tl.x; v[1].y = tl.y;
	v[2].x = br.x; v[2].y = br.y;
	v[3].x = tr.x; v[3].y = tr.y;
	return v;
}

template<class Vtx>
static constexpr std::array<Vtx, 4> mapQuadPos(std::array<Vtx, 4> v, GCRect rect)
{
	return mapQuadPos(v, {rect.x, rect.y}, {rect.x, rect.y2}, {rect.x2, rect.y2}, {rect.x2, rect.y});
}

template<class Vtx>
class QuadGeneric
{
public:
	constexpr QuadGeneric() {}

	constexpr QuadGeneric(GP bl, GP tl, GP tr, GP br)
	{
		setPos(bl, tl, tr, br);
	}

	constexpr QuadGeneric(GCRect posRect)
	{
		setPos(posRect);
	}

	constexpr QuadGeneric(GCRect posRect, GTexCRect uvRect)
	{
		setPos(posRect);
		setUV(uvRect);
	}

	constexpr void setPos(GP bl, GP tl, GP tr, GP br)
	{
		v = mapQuadPos(v, bl, tl, tr, br);
	}

	constexpr void setPos(QuadGeneric quad)
	{
		setPos(
			{quad.v[0].x, quad.v[0].y},
			{quad.v[1].x, quad.v[1].y},
			{quad.v[3].x, quad.v[3].y},
			{quad.v[2].x, quad.v[2].y});
	}

	constexpr void setPos(GCRect rect)
	{
		setPos({rect.x, rect.y}, {rect.x, rect.y2}, {rect.x2, rect.y2}, {rect.x2, rect.y});
	}

	void setPos(IG::WindowRect b, ProjectionPlane proj);
	void setPosRel(GC x, GC y, GC xSize, GC ySize);

	constexpr void setUV(GTexCRect rect)
	{
		if constexpr(!Vtx::hasTexture)
		{
			return;
		}
		else
		{
			v = mapQuadUV(v, rect);
		}
	}

	void setColor(ColorComp r, ColorComp g, ColorComp b, ColorComp a, uint32_t edges = EDGE_AI);
	void setColorRGB(ColorComp r, ColorComp g, ColorComp b, uint32_t edges = EDGE_AI);
	void setColorAlpha(ColorComp a, uint32_t edges = EDGE_AI);
	void draw(RendererCommands &r) const;
	static void draw(RendererCommands &cmds, IG::WindowRect b, ProjectionPlane proj);
	static void draw(RendererCommands &cmds, GCRect d);

protected:
	std::array<Vtx, 4> v{};
};

using Quad = QuadGeneric<Vertex>;
using TexQuad = QuadGeneric<TexVertex>;
using ColQuad = QuadGeneric<ColVertex>;
using ColTexQuad = QuadGeneric<ColTexVertex>;

std::array<Vertex, 4> makeVertArray(GCRect pos);
std::array<ColVertex, 4> makeColVertArray(GCRect pos, VertexColor col);
std::array<VertexIndex, 6> makeRectIndexArray(VertexIndex baseIdx);

template<class Vtx>
void drawQuads(RendererCommands &cmds, std::array<Vtx, 4> *quad, uint32_t quads, std::array<VertexIndex, 6> *quadIdx, uint32_t quadIdxs);

}
