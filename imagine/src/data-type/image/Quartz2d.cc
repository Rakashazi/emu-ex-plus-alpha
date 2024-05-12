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
#include <CoreGraphics/CGBitmapContext.h>
#include <CoreGraphics/CGContext.h>
#include <cassert>

namespace IG::Data
{

int Quartz2dImage::width()
{
	return CGImageGetWidth(img.get());
}

int Quartz2dImage::height()
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
		return IG::PixelFmtI8;
	else
		return IG::PixelFmtRGBA8888;
}

Quartz2dImage::Quartz2dImage(CStringView name)
{
	CGDataProviderRef dataProvider = CGDataProviderCreateWithFilename(name);
	if(!dataProvider)
	{
		logErr("error opening file: %s", name.data());
		return;
	}
	auto imgRef = CGImageCreateWithPNGDataProvider(dataProvider, nullptr, 0, kCGRenderingIntentDefault);
	CGDataProviderRelease(dataProvider);
	if(!imgRef)
	{
		logErr("error creating CGImage from file: %s", name.data());
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

void Quartz2dImage::readImage(MutablePixmapView dest)
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
}

void Quartz2dImage::releaseCGImage(CGImageRef ref)
{
	CGImageRelease(ref);
}

PixmapImage::operator bool() const
{
	return (bool)img;
}

void PixmapImage::write(MutablePixmapView dest)
{
	readImage(dest);
}

PixmapView PixmapImage::pixmapView()
{
	return PixmapView{{{(int)width(), (int)height()}, pixelFormat()}};
}

PixmapImage::operator PixmapSource()
{
	return {[this](MutablePixmapView dest){ return write(dest); }, pixmapView()};
}

bool PixmapImage::isPremultipled() const { return true; }

PixmapImage PixmapReader::loadAsset(const char *name, PixmapReaderParams, const char *appName) const
{
	return PixmapImage(FS::pathString(appContext().assetPath(appName), name));
}

}
