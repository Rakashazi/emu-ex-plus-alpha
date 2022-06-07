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

namespace IG::Gfx
{

template<class Vtx>
static void setColor(std::array<Vtx, 4> &v, VertexColor col, uint32_t edges)
{
	if(edges & EDGE_BL) v[0].color = col;
	if(edges & EDGE_TL) v[1].color = col;
	if(edges & EDGE_TR) v[3].color = col;
	if(edges & EDGE_BR) v[2].color = col;
}

template<Vertex Vtx>
void QuadGeneric<Vtx>::setPos(IG::WindowRect b, ProjectionPlane proj)
{
	setPos(proj.unProjectRect(b));
}

template<Vertex Vtx>
void QuadGeneric<Vtx>::setPosRel(float x, float y, float xSize, float ySize)
{
	setPos({{x, y}, {x+xSize, y+ySize}});
}


template<Vertex Vtx>
void QuadGeneric<Vtx>::draw(RendererCommands &cmds) const
{
	cmds.bindTempVertexBuffer();
	cmds.vertexBufferData(v.data(), sizeof(v));
	Vtx::bindAttribs(cmds, v.data());
	cmds.drawPrimitives(Primitive::TRIANGLE_STRIP, 0, 4);
}

template<Vertex Vtx>
void QuadGeneric<Vtx>::draw(RendererCommands &cmds, IG::WindowRect b, ProjectionPlane proj)
{
	draw(cmds, proj.unProjectRect(b));
}

template<Vertex Vtx>
void QuadGeneric<Vtx>::draw(RendererCommands &cmds, GCRect d)
{
	QuadGeneric<Vtx> rect{d};
	rect.draw(cmds);
}

template class QuadGeneric<Vertex2D>;
template class QuadGeneric<ColVertex>;
template class QuadGeneric<TexVertex>;
template class QuadGeneric<ColTexVertex>;

template<Vertex Vtx>
void QuadGeneric<Vtx>::setColor(VertexColor col, uint32_t edges)
{
	if constexpr(!Vtx::hasColor)
	{
		return;
	}
	else
	{
		Gfx::setColor(v, col, edges);
	}
}

template void QuadGeneric<Vertex2D>::setColor(VertexColor, uint32_t edges);
template void QuadGeneric<ColVertex>::setColor(VertexColor, uint32_t edges);
template void QuadGeneric<TexVertex>::setColor(VertexColor, uint32_t edges);
template void QuadGeneric<ColTexVertex>::setColor(VertexColor, uint32_t edges);

std::array<Vertex2D, 4> makeVertArray(GCRect pos)
{
	std::array<Vertex2D, 4> arr{};
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

template void drawQuads<Vertex2D>(RendererCommands &cmds, std::array<Vertex2D, 4> *quad, uint32_t quads, std::array<VertexIndex, 6> *quadIdx, uint32_t quadIdxs);
template void drawQuads<ColVertex>(RendererCommands &cmds, std::array<ColVertex, 4> *quad, uint32_t quads, std::array<VertexIndex, 6> *quadIdx, uint32_t quadIdxs);
template void drawQuads<TexVertex>(RendererCommands &cmds, std::array<TexVertex, 4> *quad, uint32_t quads, std::array<VertexIndex, 6> *quadIdx, uint32_t quadIdxs);
template void drawQuads<ColTexVertex>(RendererCommands &cmds, std::array<ColTexVertex, 4> *quad, uint32_t quads, std::array<VertexIndex, 6> *quadIdx, uint32_t quadIdxs);

}
