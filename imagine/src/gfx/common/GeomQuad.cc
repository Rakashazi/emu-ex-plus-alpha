/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/gfx/GeomQuad.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/gfx/ProjectionPlane.hh>

namespace Gfx
{

template<class Vtx>
static void setColor(std::array<Vtx, 4> &v, VertexColor col, uint32_t edges)
{
	if(edges & EDGE_BL) v[0].color = col;
	if(edges & EDGE_TL) v[1].color = col;
	if(edges & EDGE_TR) v[3].color = col;
	if(edges & EDGE_BR) v[2].color = col;
}

template<class Vtx>
static void setColor(std::array<Vtx, 4> &v, ColorComp r, ColorComp g, ColorComp b, ColorComp a, uint32_t edges)
{
	if(edges & EDGE_BL) v[0].color = VertexColorPixelFormat.build((uint32_t)r, (uint32_t)g, (uint32_t)b, (uint32_t)a);
	if(edges & EDGE_TL) v[1].color = VertexColorPixelFormat.build((uint32_t)r, (uint32_t)g, (uint32_t)b, (uint32_t)a);
	if(edges & EDGE_TR) v[3].color = VertexColorPixelFormat.build((uint32_t)r, (uint32_t)g, (uint32_t)b, (uint32_t)a);
	if(edges & EDGE_BR) v[2].color = VertexColorPixelFormat.build((uint32_t)r, (uint32_t)g, (uint32_t)b, (uint32_t)a);
}

template<class Vtx>
static void setColorRGB(std::array<Vtx, 4> &v, ColorComp r, ColorComp g, ColorComp b, uint32_t edges)
{
	if(edges & EDGE_BL) setColor(v, r, g, b, VertexColorPixelFormat.a(v[0].color), EDGE_BL);
	if(edges & EDGE_TL) setColor(v, r, g, b, VertexColorPixelFormat.a(v[1].color), EDGE_TL);
	if(edges & EDGE_TR) setColor(v, r, g, b, VertexColorPixelFormat.a(v[3].color), EDGE_TR);
	if(edges & EDGE_BR) setColor(v, r, g, b, VertexColorPixelFormat.a(v[2].color), EDGE_BR);
}

template<class Vtx>
static void setColorAlpha(std::array<Vtx, 4> &v, ColorComp a, uint32_t edges)
{
	if(edges & EDGE_BL) setColor(v, VertexColorPixelFormat.r(v[0].color), VertexColorPixelFormat.g(v[0].color), VertexColorPixelFormat.b(v[0].color), a, EDGE_BL);
	if(edges & EDGE_TL) setColor(v, VertexColorPixelFormat.r(v[1].color), VertexColorPixelFormat.g(v[1].color), VertexColorPixelFormat.b(v[1].color), a, EDGE_TL);
	if(edges & EDGE_TR) setColor(v, VertexColorPixelFormat.r(v[3].color), VertexColorPixelFormat.g(v[3].color), VertexColorPixelFormat.b(v[3].color), a, EDGE_TR);
	if(edges & EDGE_BR) setColor(v, VertexColorPixelFormat.r(v[2].color), VertexColorPixelFormat.g(v[2].color), VertexColorPixelFormat.b(v[2].color), a, EDGE_BR);
}

template<class Vtx>
void QuadGeneric<Vtx>::setPos(IG::WindowRect b, ProjectionPlane proj)
{
	setPos(proj.unProjectRect(b));
}

template<class Vtx>
void QuadGeneric<Vtx>::setPosRel(GC x, GC y, GC xSize, GC ySize)
{
	setPos({{x, y}, {x+xSize, y+ySize}});
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
	return mapQuadPos(arr, pos);
}

std::array<ColVertex, 4> makeColVertArray(GCRect pos, VertexColor col)
{
	std::array<ColVertex, 4> arr{};
	arr = mapQuadPos(arr, pos);
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
