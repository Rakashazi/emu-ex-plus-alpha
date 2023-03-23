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
#include <imagine/base/ApplicationContext.hh>
#include <imagine/pixmap/Pixmap.hh>
#include <CoreGraphics/CGImage.h>
#include <memory>
#include <type_traits>

namespace IG::Data
{

class Quartz2dImage
{
public:
	Quartz2dImage(CStringView path);
	void readImage(MutablePixmapView dest);
	bool hasAlphaChannel();
	bool isGrayscale();
	int width();
	int height();
	const PixelFormat pixelFormat();
	explicit operator bool() const;

protected:
	static void releaseCGImage(CGImageRef);

	struct CGImageDeleter
	{
		void operator()(CGImageRef ptr) const
		{
			releaseCGImage(ptr);
		}
	};

	using UniqueCGImage = std::unique_ptr<std::remove_pointer_t<CGImageRef>, CGImageDeleter>;
	UniqueCGImage img{};
};

using PixmapImageImpl = Quartz2dImage;

class Quartz2dImageReader
{
public:
	constexpr Quartz2dImageReader(ApplicationContext ctx):
		ctx{ctx}
	{}

protected:
	ApplicationContext ctx{};

	constexpr ApplicationContext appContext() const { return ctx; }
};

using PixmapReaderImpl = Quartz2dImageReader;

class Quartz2dImageWriter
{
public:
	constexpr Quartz2dImageWriter(ApplicationContext) {}
};

using PixmapWriterImpl = Quartz2dImageWriter;

}
