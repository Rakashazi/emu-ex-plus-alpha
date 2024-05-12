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

static_assert(__has_feature(objc_arc), "This file requires ARC");
#define LOGTAG "QuartzPNG"
#include <imagine/data-type/image/PixmapWriter.hh>
#include <imagine/pixmap/MemPixmap.hh>
#include "../../base/iphone/private.hh"
#import <UIKit/UIImage.h>

namespace IG::Data
{

bool PixmapWriter::writeToFile(PixmapView srcPix, const char *path) const
{
	IG::MemPixmap tempMemPix{{srcPix.size(), IG::PixelFmtRGB888}};
	auto pix = tempMemPix.view();
	pix.writeConverted(srcPix);
	auto provider = CGDataProviderCreateWithData(nullptr, pix.data(), pix.bytes(), nullptr);
	int bitsPerComponent = 8;
	auto colorSpace = CGColorSpaceCreateDeviceRGB();
	CGBitmapInfo bitmapInfo = kCGImageAlphaNone;
	auto imageRef = CGImageCreate(pix.w(), pix.h(), bitsPerComponent, pix.format().bitsPerPixel(), pix.pitchBytes(), colorSpace, bitmapInfo,
		provider, nullptr, NO, kCGRenderingIntentDefault);
	CGColorSpaceRelease(colorSpace);
	CGDataProviderRelease(provider);
	@autoreleasepool
	{
		UIImage *uiImage = [UIImage imageWithCGImage:imageRef];
		CGImageRelease(imageRef);
		auto pathStr = [[NSString alloc] initWithBytesNoCopy:(void*)path length:strlen(path) encoding:NSUTF8StringEncoding freeWhenDone:false];
		return [UIImagePNGRepresentation(uiImage) writeToFile:pathStr atomically:YES];
	}
}

}
