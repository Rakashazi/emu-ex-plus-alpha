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
	std::errc readImage(IG::Pixmap &dest);
	bool hasAlphaChannel();
	bool isGrayscale();
	void freeImageData();
	uint32_t width();
	uint32_t height();
	IG::PixelFormat pixelFormat() const;
	explicit operator bool() const;

private:
	jobject bitmap{};
	AndroidBitmapInfo info{};
};

class PngFile : public GfxImageSource
{
public:
	PngFile();
	~PngFile();
	std::error_code load(const char *name);
	std::error_code loadAsset(const char *name, const char *appName);
	void deinit();
	std::errc write(IG::Pixmap dest) final;
	IG::Pixmap lockPixmap() final;
	void unlockPixmap() final;
	explicit operator bool() const final;

private:
	BitmapFactoryImage png;
};
