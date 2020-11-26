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

#include <imagine/gfx/GfxLGradient.hh>
#include <imagine/util/algorithm.h>
#include <imagine/util/math/space.hh>

namespace Gfx
{

void LGradient::draw(RendererCommands &cmds) const
{
	g.draw(cmds);
}

void LGradient::setColor(ColorComp r, ColorComp g, ColorComp b)
{
	this->g.setColorRGB(r, g, b);
}

void LGradient::setTranslucent(ColorComp a)
{
	g.setColorTranslucent(a);
}

void LGradient::setColorStop(ColorComp r, ColorComp g, ColorComp b, uint32_t i)
{
	this->g.setColorRGBV(r, g, b, i*2);
	this->g.setColorRGBV(r, g, b, (i*2)+1);
}

void LGradient::setTranslucentStop(ColorComp a, uint32_t i)
{
	g.setColorTranslucentV(a, i*2);
	g.setColorTranslucentV(a, (i*2)+1);
}

void LGradient::setPos(const LGradientStopDesc *stop, uint32_t stops, GC x, GC y, GC x2, GC y2)
{
	if(stops != stops_)
	{
		assert(stops >= 2);
		stops_ = stops;
		VertexPos thickness[2]{x, x2};
		VertexPos stopPos[stops];
		iterateTimes(stops, i)
		{
			stopPos[i] = stop[i].pos;
		}
		g = {thickness, stopPos, stops};
	}

	ColVertex *v = g.v().data();
	iterateTimes(stops, i)
	{
		v[i*2].x = x;
		v[i*2].y = v[(i*2)+1].y = IG::scalePointRange(stop[i].pos, GC(0), GC(1), y, y2);
		v[(i*2)+1].x = x2;

		v[i*2].color = stop[i].color;
		v[(i*2)+1].color = stop[i].color;
	}
}

void LGradient::setPos(const LGradientStopDesc *stop, uint32_t stops, const GCRect &d)
{
	 setPos(stop, stops, d.x, d.y2, d.x2, d.y);
}

uint32_t LGradient::stops() const
{
	return stops_;
}

LGradient::operator bool() const
{
	return stops_;
}

}
