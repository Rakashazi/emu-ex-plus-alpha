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
#include <system_error>

struct png_struct_def;
struct png_info_def;

namespace IG
{
class GenericIO;
class PixelFormat;
class Pixmap;
}

namespace IG::Data
{

class PngImage
{
public:
	constexpr PngImage() = default;
	PngImage(GenericIO io);
	PngImage(PngImage &&o);
	PngImage &operator=(PngImage &&o);
	~PngImage();
	std::errc readImage(IG::Pixmap dest);
	bool hasAlphaChannel();
	bool isGrayscale();
	void freeImageData();
	uint32_t width();
	uint32_t height();
	IG::PixelFormat pixelFormat();

protected:
	png_struct_def *png{};
	png_info_def *info{};
	void setTransforms(IG::PixelFormat outFormat, png_info_def *transInfo);
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
