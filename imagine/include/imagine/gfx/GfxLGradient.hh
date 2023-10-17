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

#include <imagine/gfx/defs.hh>

namespace IG::Gfx
{

struct LGradientStopDesc
{
	float pos;
	PackedColor color;
};

enum class LGradientPadMode
{
	none, top
};

class LGradient
{
public:
	static size_t vertexSize(size_t stops, LGradientPadMode padMode)
	{
		size_t size = 2 * stops;
		if(padMode == LGradientPadMode::top)
			size += 2;
		return size;
	}

	static void write(auto &&vBuff, int vOffset, auto &&stops, WRect pos, std::optional<int> topPadding)
	{
		auto v = vBuff.data() + vOffset * 2;
		if(topPadding)
		{
			v[0].pos.x = pos.x;
			v[1].pos.x = pos.x2;
			v[0].pos.y = v[1].pos.y = *topPadding;
			v[0].color = v[1].color = stops[0].color;
			v += 2;
		}
		for(const auto &stop : stops)
		{
			v[0].pos.x = pos.x;
			v[1].pos.x = pos.x2;
			v[0].pos.y = v[1].pos.y = remap(stop.pos, 0.f, 1.f, pos.y, pos.y2);
			v[0].color = v[1].color = stop.color;
			v += 2;
		}
	}
};

}
