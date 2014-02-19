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

#include <engine-globals.h>
#include <pixmap/Pixmap.hh>
#include <data-type/image/GfxImageSource.hh>
#include <io/sys.hh>
#include <fs/sys.hh>

#define PNG_SKIP_SETJMP_CHECK
#include <png.h>

class PixelFormatDesc;

class Png
{
public:
	constexpr Png() {}
	CallResult readHeader(Io *stream);
	CallResult readImage(Io *stream, IG::Pixmap &dest);
	bool hasAlphaChannel();
	bool isGrayscale();
	void freeImageData();
	uint width();
	uint height();
	const PixelFormatDesc *pixelFormat();

private:
	png_structp png = nullptr;
	png_infop info = nullptr;
	//png_infop end;
	void setTransforms(const PixelFormatDesc &outFormat, png_infop transInfo);
	static bool supportUncommonConv;
};

class PngFile : public GfxImageSource
{
public:
	PngFile() {}
	~PngFile()
	{
		deinit();
	}
	CallResult load(Io *io);
	CallResult load(const char *name);
	CallResult loadAsset(const char *name)
	{
		return load(openAppAssetIo(name));
	}
	void deinit();
	CallResult getImage(IG::Pixmap &dest) override;
	uint width() override { return png.width(); }
	uint height() override { return png.height(); }
	const PixelFormatDesc *pixelFormat() override { return png.pixelFormat(); }

private:
	Png png;
	Io *io = nullptr;
};
