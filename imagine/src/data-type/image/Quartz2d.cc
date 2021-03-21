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
#include <imagine/base/ApplicationContext.hh>
#include <imagine/fs/FS.hh>
#include "../../base/iphone/private.hh"
#include <CoreGraphics/CGBitmapContext.h>
#include <CoreGraphics/CGContext.h>

uint32_t Quartz2dImage::width()
{
	return CGImageGetWidth(img);
}

uint32_t Quartz2dImage::height()
{
	return CGImageGetHeight(img);
}

bool Quartz2dImage::isGrayscale()
{
	return CGImageGetBitsPerPixel(img) == 8;
}

const IG::PixelFormat Quartz2dImage::pixelFormat()
{
	if(isGrayscale())
		return IG::PIXEL_FMT_I8;
	else
		return hasAlphaChannel() ? IG::PIXEL_FMT_RGBA8888 : IG::PIXEL_FMT_RGB888;
}

std::error_code Quartz2dImage::load(const char *name)
{
	freeImageData();
	CGDataProviderRef dataProvider = CGDataProviderCreateWithFilename(name);
	if(!dataProvider)
	{
		logErr("error opening file: %s", name);
		return {EINVAL, std::system_category()};
	}
	img = CGImageCreateWithPNGDataProvider(dataProvider, nullptr, 0, kCGRenderingIntentDefault);
	CGDataProviderRelease(dataProvider);
	if(!img)
	{
		logErr("error creating CGImage from file: %s", name);
		return {EINVAL, std::system_category()};
	}
	return {};
}

bool Quartz2dImage::hasAlphaChannel()
{
	auto info = CGImageGetAlphaInfo(img);
	return info == kCGImageAlphaPremultipliedLast || info == kCGImageAlphaPremultipliedFirst
		|| info == kCGImageAlphaLast || info == kCGImageAlphaFirst;
}

std::errc Quartz2dImage::readImage(IG::Pixmap dest)
{
	assert(dest.format() == pixelFormat());
	int height = this->height();
	int width = this->width();
	auto colorSpace = isGrayscale() ? Base::grayColorSpace : Base::rgbColorSpace;
	auto bitmapInfo = hasAlphaChannel() ? kCGImageAlphaPremultipliedLast : kCGImageAlphaNone;
	auto context = CGBitmapContextCreate(dest.data(), width, height, 8, dest.pitchBytes(), colorSpace, bitmapInfo);
	CGContextSetBlendMode(context, kCGBlendModeCopy);
	CGContextDrawImage(context, CGRectMake(0.0, 0.0, (CGFloat)width, (CGFloat)height), img);
	CGContextRelease(context);
	return {};
}

void Quartz2dImage::freeImageData()
{
	if(img)
	{
		CGImageRelease(img);
		img = nullptr;
	}
}

Quartz2dImage::operator bool() const
{
	return (bool)img;
}

PngFile::~PngFile()
{
	deinit();
}

std::errc PngFile::write(IG::Pixmap dest)
{
	return(png.readImage(dest));
}

IG::Pixmap PngFile::pixmapView()
{
	return {{{(int)png.width(), (int)png.height()}, png.pixelFormat()}, {}};
}

std::error_code PngFile::load(const char *name)
{
	deinit();
	return png.load(name);
}

std::error_code PngFile::loadAsset(const char *name, const char *appName)
{
	return load(FS::makePathStringPrintf("%s/%s", png.appContext().assetPath(appName).data(), name).data());
}

void PngFile::deinit()
{
	png.freeImageData();
}

PngFile::operator bool() const
{
	return (bool)png;
}
