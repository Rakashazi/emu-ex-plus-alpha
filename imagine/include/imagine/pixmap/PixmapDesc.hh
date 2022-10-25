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
#include <imagine/pixmap/PixelFormat.hh>
#include <imagine/util/Point2D.hh>

namespace IG
{

struct PixmapDesc
{
	IP size{};
	PixelFormat format{};

	constexpr int w() const { return size.x; }
	constexpr int h() const { return size.y; }
	constexpr int bytes() const { return format.pixelBytes(w() * h()); }
	constexpr PixmapDesc makeNewSize(IP newSize) const { return {newSize, format}; }
	constexpr bool operator ==(const PixmapDesc &) const = default;
};

}
