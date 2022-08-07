#pragma once

#include <imagine/config/defs.hh>
#include <imagine/base/baseDefs.hh>
#include <imagine/gfx/defs.hh>
#include <imagine/gfx/Vertex.hh>
#include <imagine/util/rectangle2.h>
#include <imagine/util/edge.h>

namespace IG::Gfx
{

class RendererCommands;
class ProjectionPlane;

template<class Vtx>
static constexpr auto mapQuadUV(std::array<Vtx, 4> v, FRect rect, Rotation r = Rotation::UP)
{
	bool rotate = false;
	if(r == Rotation::DOWN)
	{
		std::swap(rect.x, rect.x2);
		std::swap(rect.y, rect.y2);
	}
	else if(r == Rotation::RIGHT)
	{
		rotate = true;
	}
	else if(r == Rotation::LEFT)
	{
		std::swap(rect.x, rect.x2);
		std::swap(rect.y, rect.y2);
		rotate = true;
	}
	if(rotate)
	{
		v[0].u = rect.x;  v[0].v = rect.y;  //BL
		v[1].u = rect.x2; v[1].v = rect.y;  //TL
		v[2].u = rect.x;  v[2].v = rect.y2; //BR
		v[3].u = rect.x2; v[3].v = rect.y2; //TR
	}
	else
	{
		v[0].u = rect.x;  v[0].v = rect.y2; //BL
		v[1].u = rect.x;  v[1].v = rect.y;  //TL
		v[2].u = rect.x2; v[2].v = rect.y2; //BR
		v[3].u = rect.x2; v[3].v = rect.y;  //TR
	}
	return v;
}

template<class Vtx>
static constexpr std::array<Vtx, 4> mapQuadPos(std::array<Vtx, 4> v, FP bl, FP tl, FP tr, FP br)
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

template<Vertex Vtx>
class QuadGeneric
{
public:
	constexpr QuadGeneric() = default;

	constexpr QuadGeneric(FP bl, FP tl, FP tr, FP br)
	{
		setPos(bl, tl, tr, br);
	}

	constexpr QuadGeneric(GCRect posRect)
	{
		setPos(posRect);
	}

	constexpr QuadGeneric(GCRect posRect, FRect uvRect)
	{
		setPos(posRect);
		setUV(uvRect);
	}

	constexpr void setPos(FP bl, FP tl, FP tr, FP br)
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
	void setPosRel(float x, float y, float xSize, float ySize);

	constexpr void setUV(FRect rect, Rotation r = Rotation::UP)
	{
		if constexpr(!Vtx::hasTexture)
		{
			return;
		}
		else
		{
			v = mapQuadUV(v, rect, r);
		}
	}

	void setColor(VertexColor, uint32_t edges = EDGE_AI);
	void draw(RendererCommands &r) const;
	static void draw(RendererCommands &cmds, IG::WindowRect b, ProjectionPlane proj);
	static void draw(RendererCommands &cmds, GCRect d);

protected:
	std::array<Vtx, 4> v{};
};

using Quad = QuadGeneric<Vertex2D>;
using TexQuad = QuadGeneric<TexVertex>;
using ColQuad = QuadGeneric<ColVertex>;
using ColTexQuad = QuadGeneric<ColTexVertex>;

std::array<Vertex2D, 4> makeVertArray(GCRect pos);
std::array<ColVertex, 4> makeColVertArray(GCRect pos, VertexColor col);
std::array<VertexIndex, 6> makeRectIndexArray(VertexIndex baseIdx);

template<class Vtx>
void drawQuads(RendererCommands &cmds, std::array<Vtx, 4> *quad, uint32_t quads, std::array<VertexIndex, 6> *quadIdx, uint32_t quadIdxs);

}
