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

#include <imagine/config/defs.hh>
#include <imagine/gfx/Gfx.hh>
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
	LGradient() {}

	void draw(RendererCommands &r);
	void setColor(ColorComp r, ColorComp g, ColorComp b);
	void setTranslucent(ColorComp a);
	void setColorStop(ColorComp r, ColorComp g, ColorComp b, uint i);
	void setTranslucentStop(ColorComp a, uint i);
	void setPos(const LGradientStopDesc *stop, uint stops, GC x, GC y, GC x2, GC y2);
	void setPos(const LGradientStopDesc *stop, uint stops, const GCRect &d);
	uint stops();
	explicit operator bool();

protected:
	GeomQuadMesh g{};
	uint stops_ = 0;
};

}
