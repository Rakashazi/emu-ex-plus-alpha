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
#include <imagine/font/FontSettings.hh>
#include <imagine/font/GlyphMetrics.hh>
#include <imagine/io/IO.hh>
#include <imagine/pixmap/Pixmap.hh>
#include <system_error>

#ifdef __ANDROID__
#include <imagine/font/AndroidFont.hh>
#elif defined __APPLE__ && TARGET_OS_IPHONE
#include <imagine/font/UIKitFont.hh>
#else
#include <imagine/font/FreetypeFont.hh>
#endif

namespace IG
{

class GlyphImage: public GlyphImageImpl
{
public:
	using GlyphImageImpl::GlyphImageImpl;

	GlyphImage() {}
	GlyphImage(GlyphImage &&o);
	GlyphImage &operator=(GlyphImage o);
	~GlyphImage();
	static void swap(GlyphImage &a, GlyphImage &b);
	IG::Pixmap pixmap();
	void unlock();
	explicit operator bool() const;
};

class Font : public FontImpl
{
public:
	struct Glyph
	{
		GlyphImage image;
		GlyphMetrics metrics;
	};

	Font();
	Font(GenericIO io);
	Font(const char *name);
	static Font makeSystem();
	static Font makeBoldSystem();
	static Font makeFromAsset(const char *name, const char *appName);
	Font(Font &&o);
	Font &operator=(Font o);
	~Font();
	operator bool() const;
	static void swap(Font &a, Font &b);
	int minUsablePixels() const;
	Glyph glyph(int idx, FontSize &size, std::errc &ec);
	GlyphMetrics metrics(int idx, FontSize &size, std::errc &ec);
	FontSize makeSize(FontSettings settings, std::errc &ec);
};

}
