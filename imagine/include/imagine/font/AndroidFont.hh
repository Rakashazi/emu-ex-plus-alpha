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
#include <jni.h>

namespace IG
{

class AndroidGlyphImage
{
public:
	constexpr AndroidGlyphImage() {}
	constexpr AndroidGlyphImage(IG::Pixmap pixmap, jobject bitmap):
		pixmap_{pixmap},
		aBitmap{bitmap}
	{}

protected:
	IG::Pixmap pixmap_{};
	jobject aBitmap{};
};

class AndroidFont
{
protected:
	bool isBold{};
};

class AndroidFontSize
{
public:
	AndroidFontSize() {}
	AndroidFontSize(jobject paint): paint_{paint} {}
	~AndroidFontSize();
	AndroidFontSize(AndroidFontSize &&o);
	AndroidFontSize &operator=(AndroidFontSize o);
	static void swap(AndroidFontSize &a, AndroidFontSize &b);
	jobject paint() const { return paint_; }

protected:
	jobject paint_{};
};

using GlyphImageImpl = AndroidGlyphImage;
using FontImpl = AndroidFont;
using FontSize = AndroidFontSize;

}
