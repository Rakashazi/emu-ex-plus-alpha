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
#ifdef __OBJC__
#import <UIKit/UIFont.h>
#endif

namespace IG
{

class UIKitGlyphImage
{
public:
	constexpr UIKitGlyphImage() {}
	UIKitGlyphImage(IG::Pixmap pixmap, void *pixData);
	UIKitGlyphImage(UIKitGlyphImage &&o);
	UIKitGlyphImage &operator=(UIKitGlyphImage &&o);
	~UIKitGlyphImage();

protected:
	IG::Pixmap pixmap_{};
	void *pixData_{};
};

class UIKitFont
{
public:
	constexpr UIKitFont() {}
	UIKitFont(UIKitFont &&o);
	UIKitFont &operator=(UIKitFont &&o);

protected:
	bool isBold{};
};

class UIKitFontSize
{
public:
	constexpr UIKitFontSize() {}
	UIKitFontSize(void *font);
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

using GlyphImageImpl = UIKitGlyphImage;
using FontImpl = UIKitFont;
using FontSize = UIKitFontSize;

}
