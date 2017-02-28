#pragma once
#include <imagine/gfx/GeomQuad.hh>

namespace Gfx
{

template<class Vtx>
CallResult QuadGeneric<Vtx>::init(Coordinate x, Coordinate y, Coordinate x2, Coordinate y2, Coordinate x3, Coordinate y3, Coordinate x4, Coordinate y4)
{
	v[0] = Vtx(x, y); //BL
	v[1] = Vtx(x2, y2); //TL
	v[2] = Vtx(x4, y4); //BR
	v[3] = Vtx(x3, y3); //TR
	return OK;
}

template<class Vtx>
void QuadGeneric<Vtx>::deinit()
{

}

template<class Vtx>
void QuadGeneric<Vtx>::setPos(GC x, GC y, GC x2, GC y2, GC x3, GC y3, GC x4, GC y4)
{
	v[0].x = x; v[0].y = y; //BL
	v[1].x = x2; v[1].y = y2; //TL
	v[2].x = x4; v[2].y = y4; //BR
	v[3].x = x3; v[3].y = y3; //TR
}

template<class Vtx>
void QuadGeneric<Vtx>::draw(Renderer &r) const
{
	r.bindTempVertexBuffer();
	r.vertexBufferData(v.data(), sizeof(v));
	Vtx::bindAttribs(r, v.data());
	r.drawPrimitives(Primitive::TRIANGLE_STRIP, 0, 4);
}

template class QuadGeneric<Vertex>;
template class QuadGeneric<ColVertex>;
template class QuadGeneric<TexVertex>;
template class QuadGeneric<ColTexVertex>;

void TexQuad::mapImg(GTexC leftTexU, GTexC topTexV, GTexC rightTexU, GTexC bottomTexV) { Gfx::mapImg(v, leftTexU, topTexV, rightTexU, bottomTexV); };

void ColQuad::setColor(ColorComp r, ColorComp g, ColorComp b, ColorComp a, uint edges) { Gfx::setColor(v, r, g, b, a, edges); }
void ColQuad::setColorRGB(ColorComp r, ColorComp g, ColorComp b, uint edges) { Gfx::setColorRGB(v, r, g, b, edges); }
void ColQuad::setColorAlpha(ColorComp a, uint edges) { Gfx::setColorAlpha(v, a, edges); }

void ColTexQuad::mapImg(GTexC leftTexU, GTexC topTexV, GTexC rightTexU, GTexC bottomTexV) { Gfx::mapImg(v, leftTexU, topTexV, rightTexU, bottomTexV); };
void ColTexQuad::setColor(ColorComp r, ColorComp g, ColorComp b, ColorComp a, uint edges) { Gfx::setColor(v, r, g, b, a, edges); }
void ColTexQuad::setColorRGB(ColorComp r, ColorComp g, ColorComp b, uint edges) { Gfx::setColorRGB(v, r, g, b, edges); }
void ColTexQuad::setColorAlpha(ColorComp a, uint edges) { Gfx::setColorAlpha(v, a, edges); }

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

}
