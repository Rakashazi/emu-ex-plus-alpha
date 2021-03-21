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
#include <jni.h>

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
	Base::ApplicationContext app{};
};

class AndroidFont
{
public:
	constexpr AndroidFont(Base::ApplicationContext app):
		app{app}
	{}

protected:
	Base::ApplicationContext app{};
	bool isBold{};
};

class AndroidFontSize
{
public:
	constexpr AndroidFontSize() {}
	AndroidFontSize(Base::ApplicationContext, jobject paint);
	AndroidFontSize(AndroidFontSize &&o);
	AndroidFontSize &operator=(AndroidFontSize &&o);
	~AndroidFontSize();
	jobject paint() const { return paint_; }

protected:
	jobject paint_{};
	Base::ApplicationContext app{};

	void deinit();
};

using GlyphImageImpl = AndroidGlyphImage;
using FontImpl = AndroidFont;
using FontSize = AndroidFontSize;

}
