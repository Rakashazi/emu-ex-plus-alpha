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

#include <imagine/engine-globals.h>
#include <imagine/pixmap/Pixmap.hh>
#include <imagine/data-type/image/GfxImageSource.hh>
#include <imagine/io/FileIO.hh>
#include <imagine/fs/sys.hh>

#define PNG_SKIP_SETJMP_CHECK
#include <png.h>

class PixelFormatDesc;

class Png
{
public:
	constexpr Png() {}
	CallResult readHeader(GenericIO io);
	CallResult readImage(IG::Pixmap &dest);
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
	CallResult load(GenericIO io);
	CallResult load(const char *name);
	CallResult loadAsset(const char *name)
	{
		return load(openAppAssetIO(name));
	}
	void deinit();
	CallResult write(IG::Pixmap dest) override;
	IG::Pixmap lockPixmap() override;
	void unlockPixmap() override;

private:
	Png png;
};
