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
#include <imagine/fs/sys.hh>
#include <CoreGraphics/CGImage.h>

class PixelFormatDesc;

class Quartz2dImage
{
public:
	constexpr Quartz2dImage() {}
	CallResult load(const char *name);
	CallResult readImage(IG::Pixmap &dest);
	static CallResult writeImage(const IG::Pixmap &pix, const char *name);
	bool hasAlphaChannel();
	bool isGrayscale();
	void freeImageData();
	uint width();
	uint height();
	const PixelFormatDesc *pixelFormat();

private:
	CGImageRef img = nullptr;
};

class PngFile : public GfxImageSource
{
public:
	PngFile() {}
	~PngFile()
	{
		deinit();
	}
	CallResult load(const char *name);
	CallResult loadAsset(const char *name);
	void deinit();
	CallResult getImage(IG::Pixmap &dest) override;
	uint width() override { return png.width(); }
	uint height() override { return png.height(); }
	const PixelFormatDesc *pixelFormat() override { return png.pixelFormat(); }

private:
	Quartz2dImage png;
};
