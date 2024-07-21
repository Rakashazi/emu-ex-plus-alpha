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
#include <system_error>

struct png_struct_def;
struct png_info_def;

namespace IG
{
class IO;
}

namespace IG::Data
{

struct PixmapReaderParams;

class PngImage
{
public:
	constexpr PngImage() = default;
	PngImage(IO, PixmapReaderParams);
	PngImage(PngImage &&o) noexcept;
	PngImage &operator=(PngImage &&o) noexcept;
	~PngImage();
	std::errc readImage(MutablePixmapView dest);
	bool hasAlphaChannel();
	bool isGrayscale();
	void freeImageData();
	int width();
	int height();
	PixelFormat pixelFormat();

protected:
	png_struct_def *png{};
	png_info_def *info{};
	bool premultiplyAlpha{};
	void setTransforms(PixelFormat outFormat);
	static bool supportUncommonConv;
};

using PixmapImageImpl = PngImage;

class PngReader
{
public:
	constexpr PngReader(ApplicationContext ctx):
		ctx{ctx}
	{}

protected:
	ApplicationContext ctx{};

	constexpr ApplicationContext appContext() const { return ctx; }
};

using PixmapReaderImpl = PngReader;

class PngWriter
{
public:
	constexpr PngWriter(ApplicationContext) {}
};

using PixmapWriterImpl = PngWriter;

}
