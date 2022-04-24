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
#include <imagine/gfx/defs.hh>
#include <imagine/gfx/GeomQuadMesh.hh>

namespace IG::Gfx
{

struct LGradientStopDesc
{
	float pos;
	VertexColor color;
};

class LGradient
{
public:
	constexpr LGradient() = default;
	void draw(RendererCommands &r) const;
	void setColor(ColorComp r, ColorComp g, ColorComp b);
	void setTranslucent(ColorComp a);
	void setColorStop(ColorComp r, ColorComp g, ColorComp b, size_t i);
	void setTranslucentStop(ColorComp a, size_t i);
	void setPos(std::span<const LGradientStopDesc> stops, float x, float y, float x2, float y2);
	void setPos(std::span<const LGradientStopDesc> stops, GCRect d);
	int stops() const;
	explicit operator bool() const;

protected:
	GeomQuadMesh g{};
	int stops_{};
};

}
