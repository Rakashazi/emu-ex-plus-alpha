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
	constexpr PixmapDesc(WP size, PixelFormat format):
		size_{size}, format_(format)
	{}
	constexpr unsigned w() const { return size().x; }
	constexpr unsigned h() const { return size().y; }
	constexpr WP size() const { return size_; }
	constexpr PixelFormat format() const { return format_; }
	constexpr size_t bytes() const { return format().pixelBytes(w() * h()); }
	constexpr PixmapDesc makeNewSize(WP newSize) const { return {newSize, format_}; }
	constexpr bool operator ==(const PixmapDesc &rhs) const = default;

protected:
	WP size_{};
	PixelFormat format_{};
};

}
