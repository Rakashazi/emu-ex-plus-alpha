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

#include <imagine/data-type/image/PixmapReader.hh>
#include <imagine/data-type/image/PixmapSource.hh>
#include <imagine/pixmap/Pixmap.hh>
#include <imagine/logger/logger.h>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/fs/FS.hh>
#include <imagine/util/format.hh>
#include <CoreGraphics/CGBitmapContext.h>
#include <CoreGraphics/CGContext.h>
#include <cassert>

namespace IG::Data
{

uint32_t Quartz2dImage::width()
{
	return CGImageGetWidth(img.get());
}

uint32_t Quartz2dImage::height()
{
	return CGImageGetHeight(img.get());
}

bool Quartz2dImage::isGrayscale()
{
	return CGImageGetBitsPerPixel(img.get()) == 8;
}

const IG::PixelFormat Quartz2dImage::pixelFormat()
{
	if(isGrayscale())
		return IG::PIXEL_FMT_I8;
	else
		return IG::PIXEL_FMT_RGBA8888;
}

Quartz2dImage::Quartz2dImage(const char *name)
{
	CGDataProviderRef dataProvider = CGDataProviderCreateWithFilename(name);
	if(!dataProvider)
	{
		logErr("error opening file: %s", name);
		return;
	}
	auto imgRef = CGImageCreateWithPNGDataProvider(dataProvider, nullptr, 0, kCGRenderingIntentDefault);
	CGDataProviderRelease(dataProvider);
	if(!imgRef)
	{
		logErr("error creating CGImage from file: %s", name);
		return;
	}
	img.reset(imgRef);
}

bool Quartz2dImage::hasAlphaChannel()
{
	auto info = CGImageGetAlphaInfo(img.get());
	return info == kCGImageAlphaPremultipliedLast || info == kCGImageAlphaPremultipliedFirst
		|| info == kCGImageAlphaLast || info == kCGImageAlphaFirst;
}

std::errc Quartz2dImage::readImage(IG::Pixmap dest)
{
	assert(dest.format() == pixelFormat());
	int height = this->height();
	int width = this->width();
	auto colorSpace = isGrayscale() ? CGColorSpaceCreateDeviceGray() : CGColorSpaceCreateDeviceRGB();
	auto bitmapInfo = isGrayscale() ? kCGImageAlphaNone : kCGImageAlphaPremultipliedLast;
	auto context = CGBitmapContextCreate(dest.data(), width, height, 8, dest.pitchBytes(), colorSpace, bitmapInfo);
	CGColorSpaceRelease(colorSpace);
	CGContextSetBlendMode(context, kCGBlendModeCopy);
	CGContextDrawImage(context, CGRectMake(0.0, 0.0, (CGFloat)width, (CGFloat)height), img.get());
	CGContextRelease(context);
	return {};
}

void Quartz2dImage::releaseCGImage(CGImageRef ref)
{
	CGImageRelease(ref);
}

PixmapImage::operator bool() const
{
	return (bool)img;
}

std::errc PixmapImage::write(IG::Pixmap dest)
{
	return(readImage(dest));
}

IG::Pixmap PixmapImage::pixmapView()
{
	return {{{(int)width(), (int)height()}, pixelFormat()}, {}};
}

PixmapImage::operator PixmapSource()
{
	return {[this](IG::Pixmap dest){ return write(dest); }, pixmapView()};
}

PixmapImage PixmapReader::loadAsset(const char *name, const char *appName) const
{
	return PixmapImage(IG::formatToPathString("{}/{}", appContext().assetPath(appName).data(), name).data());
}

}
