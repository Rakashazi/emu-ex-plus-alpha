#pragma once

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

#include <imagine/gfx/GeomQuadMesh.hh>

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

	void setColor(ColorComp r, ColorComp g, ColorComp b)
	{
		this->g.setColorRGB(r, g, b);
	}

	void setTranslucent(ColorComp a)
	{
		g.setColorTranslucent(a);
	}

	void setColorStop(ColorComp r, ColorComp g, ColorComp b, uint i)
	{
		this->g.setColorRGBV(r, g, b, i*2);
		this->g.setColorRGBV(r, g, b, (i*2)+1);
	}

	void setTranslucentStop(ColorComp a, uint i)
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

	void setPos(const GCRect &d)
	{
		 setPos(d.x, d.y2, d.x2, d.y);
	}

private:
	GeomQuadMesh g;
	uint stops = 0;
	const LGradientStopDesc *stop = nullptr;
};

}
