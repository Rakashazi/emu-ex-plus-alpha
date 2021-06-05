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
#include <CoreGraphics/CGImage.h>
#include <system_error>

namespace IG
{
class PixelFormat;
class Pixmap;
}

namespace IG::Data
{

class Quartz2dImage
{
public:
	constexpr Quartz2dImage(Base::ApplicationContext ctx):
		ctx{ctx}
	{}
	~Quartz2dImage();
	std::error_code load(const char *name);
	std::errc readImage(IG::Pixmap dest);
	bool hasAlphaChannel();
	bool isGrayscale();
	void freeImageData();
	uint32_t width();
	uint32_t height();
	const IG::PixelFormat pixelFormat();
	explicit operator bool() const;
	constexpr Base::ApplicationContext appContext() const { return ctx; }

protected:
	CGImageRef img{};
	Base::ApplicationContext ctx{};
};

using PixmapReaderImpl = Quartz2dImage;

class Quartz2dImageWriter
{
public:
	constexpr Quartz2dImageWriter(Base::ApplicationContext) {}
};

using PixmapWriterImpl = Quartz2dImageWriter;

}
