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

#define thisModuleName "quartzpng"

#include "Quartz2d.hh"
#include <assert.h>
#include <logger/interface.h>
#include <base/Base.hh>
#include <base/iphone/private.hh>
#include <mem/interface.h>
#include <util/pixel.h>
#include <util/strings.h>
#include <CoreGraphics/CGBitmapContext.h>
#include <CoreGraphics/CGContext.h>

uint Quartz2dImage::width()
{
	return CGImageGetWidth(img);
}

uint Quartz2dImage::height()
{
	return CGImageGetHeight(img);
}

bool Quartz2dImage::isGrayscale()
{
	return CGImageGetBitsPerPixel(img) == 8;
}

const PixelFormatDesc *Quartz2dImage::pixelFormat()
{
	if(isGrayscale())
		return &PixelFormatI8;
	else
		return hasAlphaChannel() ? &PixelFormatRGBA8888 : &PixelFormatRGB888;
}

CallResult Quartz2dImage::load(const char *name)
{
	freeImageData();
	CGDataProviderRef dataProvider = CGDataProviderCreateWithFilename(name);
	if(!dataProvider)
	{
		logErr("error creating opening file: %s", name);
		return INVALID_PARAMETER;
	}
	img = CGImageCreateWithPNGDataProvider(dataProvider, nullptr, 0, kCGRenderingIntentDefault);
	CGDataProviderRelease(dataProvider);
	if(!img)
	{
		logErr("error creating CGImage from file: %s", name);
		return INVALID_PARAMETER;
	}
	return OK;
}

bool Quartz2dImage::hasAlphaChannel()
{
	auto info = CGImageGetAlphaInfo(img);
	return info == kCGImageAlphaPremultipliedLast || info == kCGImageAlphaPremultipliedFirst
		|| info == kCGImageAlphaLast || info == kCGImageAlphaFirst;
}

CallResult Quartz2dImage::readImage(void* buffer, uint pitch, const PixelFormatDesc &outFormat)
{
	int height = this->height();
	int width = this->width();
	auto colorSpace = isGrayscale() ? Base::grayColorSpace : Base::rgbColorSpace;
	auto bitmapInfo = hasAlphaChannel() ? kCGImageAlphaPremultipliedLast : kCGImageAlphaNone;
	auto context = CGBitmapContextCreate(buffer, width, height, 8, pitch, colorSpace, bitmapInfo);
	CGContextDrawImage(context, CGRectMake(0.0, 0.0, (CGFloat)width, (CGFloat)height), img);
	CGContextRelease(context);
	return OK;
}

void Quartz2dImage::freeImageData()
{
	if(img)
	{
		CGImageRelease(img);
		img = nullptr;
	}
}

CallResult PngFile::getImage(Pixmap &dest)
{
	return(png.readImage(dest.data, dest.pitch, dest.format));
}

CallResult PngFile::load(const char *name)
{
	deinit();
	return png.load(name);
}

CallResult PngFile::loadAsset(const char *name)
{
	FsSys::cPath fullPath;
	string_printf(fullPath, "%s/%s", Base::appPath, name);
	return load(fullPath);
}

void PngFile::deinit()
{
	png.freeImageData();
}
