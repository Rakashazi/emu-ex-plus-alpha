#pragma once
#include <gfx/GeomQuad.hh>
template<class Vtx>
CallResult GfxQuadGeneric<Vtx>::init(Coordinate x, Coordinate y, Coordinate x2, Coordinate y2, Coordinate x3, Coordinate y3, Coordinate x4, Coordinate y4)
{
	v[0] = Vtx(x, y); //BL
	v[1] = Vtx(x2, y2); //TL
	v[2] = Vtx(x4, y4); //BR
	v[3] = Vtx(x3, y3); //TR
	return OK;
}

template<class Vtx>
void GfxQuadGeneric<Vtx>::deinit()
{

}

template<class Vtx>
void GfxQuadGeneric<Vtx>::setPos(GC x, GC y, GC x2, GC y2, GC x3, GC y3, GC x4, GC y4)
{
	v[0].x = x; v[0].y = y; //BL
	v[1].x = x2; v[1].y = y2; //TL
	v[2].x = x4; v[2].y = y4; //BR
	v[3].x = x3; v[3].y = y3; //TR
}

template<class Vtx>
void GfxQuadGeneric<Vtx>::draw() const
{
	if(!Vtx::hasTexture)
		Gfx::setActiveTexture(0);
	Vtx::draw(v, Gfx::TRIANGLE_STRIP, 4);
}

template class GfxQuadGeneric<Vertex>;
template class GfxQuadGeneric<ColVertex>;
template class GfxQuadGeneric<TexVertex>;
template class GfxQuadGeneric<ColTexVertex>;

void GfxTexQuad::mapImg(const GfxBufferImage &img) { ::mapImg(v, &img.textureDesc()); };
void GfxTexQuad::mapImg(GTexC leftTexU, GTexC topTexV, GTexC rightTexU, GTexC bottomTexV) { ::mapImg(v, leftTexU, topTexV, rightTexU, bottomTexV); };

void GfxColQuad::setColor(GColor r, GColor g, GColor b, GColor a, uint edges) { ::setColor(v, r, g, b, a, edges); }
void GfxColQuad::setColorRGB(GColor r, GColor g, GColor b, uint edges) { ::setColorRGB(v, r, g, b, edges); }
void GfxColQuad::setColorAlpha(GColor a, uint edges) { ::setColorAlpha(v, a, edges); }

void GfxColTexQuad::mapImg(const GfxBufferImage &img) { ::mapImg(v, &img.textureDesc()); };
void GfxColTexQuad::mapImg(GTexC leftTexU, GTexC topTexV, GTexC rightTexU, GTexC bottomTexV) { ::mapImg(v, leftTexU, topTexV, rightTexU, bottomTexV); };
void GfxColTexQuad::setColor(GColor r, GColor g, GColor b, GColor a, uint edges) { ::setColor(v, r, g, b, a, edges); }
void GfxColTexQuad::setColorRGB(GColor r, GColor g, GColor b, uint edges) { ::setColorRGB(v, r, g, b, edges); }
void GfxColTexQuad::setColorAlpha(GColor a, uint edges) { ::setColorAlpha(v, a, edges); }
