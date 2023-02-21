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
#include <imagine/util/ranges.hh>
#include <imagine/util/math/math.hh>

namespace IG::Gfx
{

void LGradient::draw(RendererCommands &cmds) const
{
	g.draw(cmds);
}

void LGradient::setColor(PackedColor c)
{
	g.setColor(c);
}

void LGradient::setColorStop(PackedColor c, size_t i)
{
	g.setColorV(c, i*2);
	g.setColorV(c, (i*2)+1);
}

void LGradient::setPos(std::span<const LGradientStopDesc> stops, int x, int y, int x2, int y2)
{
	if(stops.size() != (size_t)stops_)
	{
		assert(stops.size() >= 2);
		stops_ = stops.size();
		int thickness[2]{x, x2};
		int stopPos[stops.size()];
		for(auto i : iotaCount(stops.size()))
		{
			stopPos[i] = stops[i].pos;
		}
		g = {thickness, {stopPos, stops.size()}};
	}

	auto *v = g.v().data();
	for(auto i : iotaCount(stops.size()))
	{
		v[i*2].pos.x = x;
		v[i*2].pos.y = v[(i*2)+1].pos.y = IG::remap(stops[i].pos, 0.f, 1.f, y, y2);
		v[(i*2)+1].pos.x = x2;

		v[i*2].color = stops[i].color;
		v[(i*2)+1].color = stops[i].color;
	}
}

void LGradient::setPos(std::span<const LGradientStopDesc> stops, WRect d)
{
	 setPos(stops, d.x, d.y, d.x2, d.y2);
}

int LGradient::stops() const
{
	return stops_;
}

LGradient::operator bool() const
{
	return stops_;
}

}
