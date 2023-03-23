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
#include <span>

namespace IG::Gfx
{

struct LGradientStopDesc
{
	float pos;
	PackedColor color;
};

class LGradient
{
public:
	constexpr LGradient() = default;
	void draw(RendererCommands &r) const;
	void setColor(PackedColor);
	void setColorStop(PackedColor, size_t i);
	void setPos(std::span<const LGradientStopDesc> stops, int x, int y, int x2, int y2);
	void setPos(std::span<const LGradientStopDesc> stops, WRect d);
	int stops() const;
	explicit operator bool() const;
	const GeomQuadMesh &mesh() const { return g; }

protected:
	GeomQuadMesh g{};
	int stops_{};
};

}
