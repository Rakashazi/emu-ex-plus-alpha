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

#include <jni.h>
#include <imagine/config/defs.hh>
#include <imagine/pixmap/Pixmap.hh>
#include <imagine/data-type/image/GfxImageSource.hh>
#include <android/bitmap.h>
#include <system_error>

class PixelFormatDesc;

class BitmapFactoryImage
{
public:
	constexpr BitmapFactoryImage() {}
	std::error_code load(const char *name);
	std::error_code loadAsset(const char *name);
	std::error_code readImage(IG::Pixmap &dest);
	static CallResult writeImage(const IG::Pixmap &pix, const char *name);
	bool hasAlphaChannel();
	bool isGrayscale();
	void freeImageData();
	uint width();
	uint height();
	IG::PixelFormat pixelFormat() const;

private:
	jobject bitmap{};
	AndroidBitmapInfo info{};
};

class PngFile : public GfxImageSource
{
public:
	PngFile() {}
	~PngFile()
	{
		deinit();
	}
	std::error_code load(const char *name);
	std::error_code loadAsset(const char *name);
	void deinit();
	std::error_code write(IG::Pixmap &dest) override;
	IG::Pixmap lockPixmap() override;
	void unlockPixmap() override;

private:
	BitmapFactoryImage png;
};
