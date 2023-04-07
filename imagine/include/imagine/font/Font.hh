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

#ifdef __ANDROID__
#include <imagine/font/AndroidFont.hh>
#elif defined __APPLE__ && TARGET_OS_IPHONE
#include <imagine/font/UIKitFont.hh>
#else
#include <imagine/font/FreetypeFont.hh>
#endif

#include <imagine/font/FontSettings.hh>
#include <imagine/font/GlyphMetrics.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/pixmap/Pixmap.hh>

namespace IG
{

class IO;

namespace Data
{
class PixmapSource;
}

class GlyphImage: public GlyphImageImpl
{
public:
	using GlyphImageImpl::GlyphImageImpl;
	PixmapView pixmap();
	explicit operator bool() const;
	operator Data::PixmapSource();
};

class Font : public FontImpl
{
public:
	struct Glyph
	{
		GlyphImage image;
		GlyphMetrics metrics;
	};

	using FontImpl::FontImpl;
	operator bool() const;
	int minUsablePixels() const;
	Glyph glyph(int idx, FontSize &);
	GlyphMetrics metrics(int idx, FontSize &);
	FontSize makeSize(FontSettings settings);
};

class FontManager : public FontManagerImpl
{
public:
	using FontManagerImpl::FontManagerImpl;
	Font makeFromFile(IO) const;
	Font makeFromFile(const char *name) const;
	Font makeSystem() const;
	Font makeBoldSystem() const;
	Font makeFromAsset(const char *name, const char *appName = ApplicationContext::applicationName) const;
};


}
