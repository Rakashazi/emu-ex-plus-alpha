#pragma once

#include <gfx/GeomQuadMesh.hh>

namespace Gfx
{

struct LGradientStopDesc
{
	GC pos;
	VertexColor color;
};

class LGradient
{
public:
	constexpr LGradient() { }
	CallResult init(uint stops, const LGradientStopDesc *stop, Coordinate x1, Coordinate x2)
	{
		assert(stops >= 2);
		var_selfs(stop);
		var_selfs(stops);

		VertexPos thickness[2] = { x1, x2 };
		VertexPos stopPos[stops];
		iterateTimes(stops, i)
		{
			stopPos[i] = stop[i].pos;
		}

		g.init(thickness, stopPos, stops);

		ColVertex *v = g.v;
		iterateTimes(stops, i)
		{
			v[i*2].color = stop[i].color;
			v[(i*2)+1].color = stop[i].color;
		}

		return OK;
	}


	void deinit()
	{
		g.deinit();
	}

	void draw()
	{
		g.draw();
	}

	void setColor(GColor r, GColor g, GColor b)
	{
		this->g.setColorRGB(r, g, b);
	}

	void setTranslucent(GColor a)
	{
		g.setColorTranslucent(a);
	}

	void setColorStop(GColor r, GColor g, GColor b, uint i)
	{
		this->g.setColorRGBV(r, g, b, i*2);
		this->g.setColorRGBV(r, g, b, (i*2)+1);
	}

	void setTranslucentStop(GColor a, uint i)
	{
		g.setColorTranslucentV(a, i*2);
		g.setColorTranslucentV(a, (i*2)+1);
	}

	void setPos(GC x, GC y, GC x2, GC y2)
	{
		ColVertex *v = g.v;
		iterateTimes(stops, i)
		{
			v[i*2].x = x;
			v[i*2].y = v[(i*2)+1].y = IG::scalePointRange(stop[i].pos, GC(0), GC(1), y, y2);
			v[(i*2)+1].x = x2;
		}
		//g.setPos(x, y, x2, y2);
	}

private:
	GeomQuadMesh g;
	uint stops = 0;
	const LGradientStopDesc *stop = 0;
};

}
