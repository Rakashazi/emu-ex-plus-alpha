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

#include <imagine/pixmap/Pixmap.hh>
#include <imagine/util/DelegateFunc.hh>

namespace IG::Data
{

class PixmapSource
{
public:
	using WriteDelegate = DelegateFunc<void(MutablePixmapView dest)>;

	constexpr PixmapSource() = default;
	constexpr PixmapSource(PixmapView pix):pix{pix} {}
	constexpr PixmapSource(WriteDelegate del, PixmapView pix):
		writeDel{del}, pix{pix} {}
	constexpr void write(MutablePixmapView dest) { writeDel(dest); }
	constexpr PixmapView pixmapView() { return pix; }

protected:
	WriteDelegate writeDel{};
	PixmapView pix{};
};

}
