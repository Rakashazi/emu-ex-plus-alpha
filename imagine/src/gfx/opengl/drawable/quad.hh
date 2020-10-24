#pragma once
#include <imagine/gfx/GeomQuad.hh>
#include <imagine/gfx/RendererCommands.hh>

namespace Gfx
{

template<class Vtx>
void QuadGeneric<Vtx>::setPos(IG::WindowRect b, ProjectionPlane proj)
{
	auto pos = proj.unProjectRect(b);
	setPos({pos.x, pos.y, pos.x2, pos.y2});
}

template<class Vtx>
void QuadGeneric<Vtx>::setPosRel(GC x, GC y, GC xSize, GC ySize)
{
	setPos({x, y, x+xSize, y+ySize});
}


template<class Vtx>
void QuadGeneric<Vtx>::draw(RendererCommands &cmds) const
{
	cmds.bindTempVertexBuffer();
	cmds.vertexBufferData(v.data(), sizeof(v));
	Vtx::bindAttribs(cmds, v.data());
	cmds.drawPrimitives(Primitive::TRIANGLE_STRIP, 0, 4);
}

template<class Vtx>
void QuadGeneric<Vtx>::draw(RendererCommands &cmds, IG::WindowRect b, ProjectionPlane proj)
{
	draw(cmds, proj.unProjectRect(b));
}

template<class Vtx>
void QuadGeneric<Vtx>::draw(RendererCommands &cmds, GCRect d)
{
	QuadGeneric<Vtx> rect{d};
	rect.draw(cmds);
}

template class QuadGeneric<Vertex>;
template class QuadGeneric<ColVertex>;
template class QuadGeneric<TexVertex>;
template class QuadGeneric<ColTexVertex>;

template<class Vtx>
void QuadGeneric<Vtx>::setColor(ColorComp r, ColorComp g, ColorComp b, ColorComp a, uint32_t edges)
{
	if constexpr(!Vtx::hasColor)
	{
		return;
	}
	else
	{
		Gfx::setColor(v, r, g, b, a, edges);
	}
}

template<class Vtx>
void QuadGeneric<Vtx>::setColorRGB(ColorComp r, ColorComp g, ColorComp b, uint32_t edges)
{
	if constexpr(!Vtx::hasColor)
	{
		return;
	}
	else
	{
		Gfx::setColorRGB(v, r, g, b, edges);
	}
}

template<class Vtx>
void QuadGeneric<Vtx>::setColorAlpha(ColorComp a, uint32_t edges)
{
	if constexpr(!Vtx::hasColor)
	{
		return;
	}
	else
	{
		Gfx::setColorAlpha(v, a, edges);
	}
}

std::array<Vertex, 4> makeVertArray(GCRect pos)
{
	std::array<Vertex, 4> arr{};
	setPos(arr, pos.x, pos.y, pos.x2, pos.y2);
	return arr;
}

std::array<ColVertex, 4> makeColVertArray(GCRect pos, VertexColor col)
{
	std::array<ColVertex, 4> arr{};
	setPos(arr, pos.x, pos.y, pos.x2, pos.y2);
	setColor(arr, col, EDGE_ALL);
	return arr;
}

std::array<VertexIndex, 6> makeRectIndexArray(VertexIndex baseIdx)
{
	baseIdx *= 4;
	return
	{{
		baseIdx,
		VertexIndex(baseIdx+1),
		VertexIndex(baseIdx+3),
		baseIdx,
		VertexIndex(baseIdx+3),
		VertexIndex(baseIdx+2),
	}};
}

template<class Vtx>
void drawQuads(RendererCommands &cmds, std::array<Vtx, 4> *quad, uint32_t quads, std::array<VertexIndex, 6> *quadIdx, uint32_t quadIdxs)
{
	cmds.bindTempVertexBuffer();
	cmds.vertexBufferData(quad[0].data(), sizeof(quad[0]) * quads);
	Vtx::bindAttribs(cmds, quad[0].data());
	cmds.drawPrimitiveElements(Primitive::TRIANGLE, quadIdx[0].data(), quadIdxs * 6);
}

template void drawQuads<Vertex>(RendererCommands &cmds, std::array<Vertex, 4> *quad, uint32_t quads, std::array<VertexIndex, 6> *quadIdx, uint32_t quadIdxs);
template void drawQuads<ColVertex>(RendererCommands &cmds, std::array<ColVertex, 4> *quad, uint32_t quads, std::array<VertexIndex, 6> *quadIdx, uint32_t quadIdxs);
template void drawQuads<TexVertex>(RendererCommands &cmds, std::array<TexVertex, 4> *quad, uint32_t quads, std::array<VertexIndex, 6> *quadIdx, uint32_t quadIdxs);
template void drawQuads<ColTexVertex>(RendererCommands &cmds, std::array<ColTexVertex, 4> *quad, uint32_t quads, std::array<VertexIndex, 6> *quadIdx, uint32_t quadIdxs);

}
