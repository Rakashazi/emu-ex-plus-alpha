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

#define LOGTAG "QuartzPNG"

#include <imagine/data-type/image/Quartz2d.hh>
#include <assert.h>
#include <imagine/logger/logger.h>
#include <imagine/base/Base.hh>
#include "../../base/iphone/private.hh"
#include <imagine/util/pixel.h>
#include <imagine/util/strings.h>
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
		logErr("error opening file: %s", name);
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

CallResult Quartz2dImage::readImage(IG::Pixmap &dest)
{
	assert(dest.format.id == pixelFormat()->id);
	int height = this->height();
	int width = this->width();
	auto colorSpace = isGrayscale() ? Base::grayColorSpace : Base::rgbColorSpace;
	auto bitmapInfo = hasAlphaChannel() ? kCGImageAlphaPremultipliedLast : kCGImageAlphaNone;
	auto context = CGBitmapContextCreate(dest.data, width, height, 8, dest.pitch, colorSpace, bitmapInfo);
	CGContextSetBlendMode(context, kCGBlendModeCopy);
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

CallResult PngFile::getImage(IG::Pixmap &dest)
{
	return(png.readImage(dest));
}

CallResult PngFile::load(const char *name)
{
	deinit();
	return png.load(name);
}

CallResult PngFile::loadAsset(const char *name)
{
	return load(makeFSPathStringPrintf("%s/%s", Base::assetPath(), name).data());
}

void PngFile::deinit()
{
	png.freeImageData();
}
