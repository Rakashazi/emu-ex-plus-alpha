#pragma once

#include <gfx/Gfx.hh>
#include <util/area2.h>
#include <util/rectangle2.h>
#include <gfx/GfxBufferImage.hh>
#include <util/edge.h>

namespace Gfx
{

template<class Vtx>
class QuadGeneric
{
public:
	constexpr QuadGeneric() { }
	CallResult init(GC x, GC y, GC x2, GC y2, GC x3, GC y3, GC x4, GC y4);
	void deinit();
	void setPos(GC x, GC y, GC x2, GC y2, GC x3, GC y3, GC x4, GC y4);
	void draw() const;

	// as rectangle
	CallResult init(GC x, GC y, GC x2, GC y2)
	{
		return init(x, y,  x, y2,  x2, y2,  x2, y);
	}

	CallResult init(const Rect2<GC> &d)
	{
		return init(d.x, d.y, d.x2, d.y2);
	}

	void setPos(GC x, GC y, GC x2, GC y2)
	{
		setPos(x, y,  x, y2,  x2, y2,  x2, y);
	}

	void setPos(const QuadGeneric &quad)
	{
		memcpy(v, quad.v, sizeof(v));
	}

	void setPos(const Area &area)
	{
		setPos(area.xPos(LB2DO), area.yPos(LB2DO), area.xPos(RT2DO), area.yPos(RT2DO));
	}

	void setPos(const Rect2<int> &b)
	{
		using namespace Gfx;
		setPos(gXPos(b, LB2DO), gYPos(b, LB2DO),
				gXPos(b, RT2DO), gYPos(b, RT2DO));
	}

	void setPosRel(GC x, GC y, GC xSize, GC ySize)
	{
		setPos(x, y, x+xSize, y+ySize);
	}

	static void draw(const Rect2<int> &b)
	{
		using namespace Gfx;
		Rect2<GC> d(gXPos(b, LB2DO), gYPos(b, LB2DO),
				gXPos(b, RT2DO), gYPos(b, RT2DO));
		draw(d);
	}

	static void draw(const Rect2<GC> &d)
	{
		QuadGeneric<Vtx> rect;
		rect.init(d);
		rect.draw();
	}

protected:
	Vtx v[4];
};

typedef QuadGeneric<Vertex> Quad;

class TexQuad : public QuadGeneric<TexVertex>
{
public:
	constexpr TexQuad() { }
	void mapImg(const BufferImage &img);
	void mapImg(GTexC leftTexU, GTexC topTexV, GTexC rightTexU, GTexC bottomTexV);
};

class ColQuad : public QuadGeneric<ColVertex>
{
public:
	constexpr ColQuad() { }
	void setColor(GColor r, GColor g, GColor b, GColor a, uint edges = EDGE_AI);
	void setColorRGB(GColor r, GColor g, GColor b, uint edges = EDGE_AI);
	void setColorAlpha(GColor a, uint edges = EDGE_AI);
};

class ColTexQuad : public QuadGeneric<ColTexVertex>
{
public:
	constexpr ColTexQuad() { }
	void mapImg(const BufferImage &img);
	void mapImg(GTexC leftTexU, GTexC topTexV, GTexC rightTexU, GTexC bottomTexV);
	void setColor(GColor r, GColor g, GColor b, GColor a, uint edges = EDGE_AI);
	void setColorRGB(GColor r, GColor g, GColor b, uint edges = EDGE_AI);
	void setColorAlpha(GColor a, uint edges = EDGE_AI);
};

}
