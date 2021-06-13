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

#include <imagine/config/defs.hh>
#include <imagine/pixmap/Pixmap.hh>
#include <CoreFoundation/CFBase.h>
#include <CoreGraphics/CGColor.h>
#ifdef __OBJC__
#import <UIKit/UIFont.h>
#endif

namespace Base
{
class ApplicationContext;
}

namespace IG
{

class UIKitGlyphImage
{
public:
	constexpr UIKitGlyphImage() {}
	constexpr UIKitGlyphImage(IG::Pixmap pixmap, void *pixData):
		pixmap_{pixmap}, pixData_{pixData} {}
	UIKitGlyphImage(UIKitGlyphImage &&o);
	UIKitGlyphImage &operator=(UIKitGlyphImage &&o);
	~UIKitGlyphImage();

protected:
	IG::Pixmap pixmap_{};
	void *pixData_{};

	void deinit();
};

class UIKitFont
{
public:
	constexpr UIKitFont() {}
	constexpr UIKitFont(CGColorSpaceRef grayColorSpace, CGColorRef textColor, bool isBold = false):
		grayColorSpace{grayColorSpace}, textColor{textColor}, isBold{isBold} {}

protected:
	CGColorSpaceRef grayColorSpace{};
	CGColorRef textColor{};
	bool isBold{};
};

class UIKitFontSize
{
public:
	constexpr UIKitFontSize() {}
	constexpr UIKitFontSize(void *font): font_{font} {}
	UIKitFontSize(UIKitFontSize &&o);
	UIKitFontSize &operator=(UIKitFontSize &&o);
	~UIKitFontSize();
	#ifdef __OBJC__
	UIFont *font() { assert(font_); return (__bridge UIFont*)font_; }
	#endif

protected:
	void *font_{}; // UIFont in ObjC

	void deinit();
};

class UIKitFontFontManager
{
public:
	UIKitFontFontManager(Base::ApplicationContext);
	~UIKitFontFontManager();

protected:
	CGColorSpaceRef grayColorSpace{}; // owner
	CGColorRef textColor{}; // owner
};

using GlyphImageImpl = UIKitGlyphImage;
using FontImpl = UIKitFont;
using FontSize = UIKitFontSize;
using FontManagerImpl = UIKitFontFontManager;

}
