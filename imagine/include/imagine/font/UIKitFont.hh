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
#include <imagine/font/FontSettings.hh>
#include <CoreFoundation/CFBase.h>
#include <CoreGraphics/CGColor.h>
#ifdef __OBJC__
#import <UIKit/UIFont.h>
#endif

namespace IG
{

class ApplicationContext;

class UIKitGlyphImage
{
public:
	constexpr UIKitGlyphImage() = default;
	constexpr UIKitGlyphImage(PixmapView pixmap, void *pixData):
		pixmap_{pixmap}, pixData_{pixData} {}
	UIKitGlyphImage(UIKitGlyphImage &&o) noexcept;
	UIKitGlyphImage &operator=(UIKitGlyphImage &&o) noexcept;
	~UIKitGlyphImage();

protected:
	PixmapView pixmap_{};
	void *pixData_{};

	void deinit();
};

class UIKitFont
{
public:
	constexpr UIKitFont() = default;
	constexpr UIKitFont(CGColorSpaceRef grayColorSpace, CGColorRef textColor, FontWeight weight = {}):
		grayColorSpace{grayColorSpace}, textColor{textColor}, weight{weight} {}

protected:
	CGColorSpaceRef grayColorSpace{};
	CGColorRef textColor{};
	FontWeight weight{};
};

class UIKitFontSize
{
public:
	constexpr UIKitFontSize() = default;
	constexpr UIKitFontSize(void *font): font_{font} {}
	UIKitFontSize(UIKitFontSize &&o) noexcept;
	UIKitFontSize &operator=(UIKitFontSize &&o) noexcept;
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
	UIKitFontFontManager(ApplicationContext);
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
