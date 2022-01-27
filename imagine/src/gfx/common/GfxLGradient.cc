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

namespace IG::Gfx
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

void LGradient::setPos(std::span<const LGradientStopDesc> stops, float x, float y, float x2, float y2)
{
	if(stops.size() != stops_)
	{
		assert(stops.size() >= 2);
		stops_ = stops.size();
		VertexPos thickness[2]{x, x2};
		VertexPos stopPos[stops.size()];
		iterateTimes(stops.size(), i)
		{
			stopPos[i] = stops[i].pos;
		}
		g = {thickness, stopPos, (uint32_t)stops.size()};
	}

	ColVertex *v = g.v().data();
	iterateTimes(stops.size(), i)
	{
		v[i*2].x = x;
		v[i*2].y = v[(i*2)+1].y = IG::remap(stops[i].pos, 0.f, 1.f, y, y2);
		v[(i*2)+1].x = x2;

		v[i*2].color = stops[i].color;
		v[(i*2)+1].color = stops[i].color;
	}
}

void LGradient::setPos(std::span<const LGradientStopDesc> stops, GCRect d)
{
	 setPos(stops, d.x, d.y2, d.x2, d.y);
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
