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
#include <imagine/util/rectangle2.h>
#include <compare>

namespace IG
{

class PixmapDesc
{
public:
	constexpr PixmapDesc() {}
	constexpr PixmapDesc(WP size, PixelFormat format): w_{(uint32_t)size.x}, h_{(uint32_t)size.y}, format_(format) {}
	constexpr uint32_t w() const { return w_; }
	constexpr uint32_t h() const { return h_; }
	constexpr WP size() const { return {(int)w(), (int)h()}; }
	constexpr PixelFormat format() const { return format_; }
	constexpr size_t pixelBytes() const { return format().pixelBytes(w() * h()); }
	constexpr bool operator ==(const PixmapDesc &rhs) const = default;

protected:
	uint32_t w_ = 0, h_ = 0;
	PixelFormat format_{};
};

}
