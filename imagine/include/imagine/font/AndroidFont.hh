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
#include <imagine/base/ApplicationContext.hh>
#include <imagine/pixmap/Pixmap.hh>
#include <imagine/util/jni.hh>

namespace IG
{

class AndroidGlyphImage
{
public:
	constexpr AndroidGlyphImage() {}
	AndroidGlyphImage(Base::ApplicationContext, IG::Pixmap pixmap, jobject bitmap);
	AndroidGlyphImage(AndroidGlyphImage &&o);
	AndroidGlyphImage &operator=(AndroidGlyphImage &&o);
	~AndroidGlyphImage();

protected:
	IG::Pixmap pixmap_{};
	jobject aBitmap{};
	Base::ApplicationContext ctx{};
};

class AndroidFont
{
public:
	constexpr AndroidFont(Base::ApplicationContext ctx):
		ctx{ctx}
	{}

protected:
	Base::ApplicationContext ctx{};
	bool isBold{};
};

class AndroidFontSize
{
public:
	constexpr AndroidFontSize() {}
	AndroidFontSize(JNI::UniqueGlobalRef paint);
	jobject paint() const { return paint_; }

protected:
	JNI::UniqueGlobalRef paint_{};
};

using GlyphImageImpl = AndroidGlyphImage;
using FontImpl = AndroidFont;
using FontSize = AndroidFontSize;

}
