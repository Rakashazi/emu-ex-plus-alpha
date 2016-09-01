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
#include <imagine/util/container/ArrayList.hh>
#include <system_error>
#include <array>
#include <ft2build.h>
#include FT_FREETYPE_H

namespace IG
{

class FreetypeGlyphImage
{
public:
	constexpr FreetypeGlyphImage(){}
	constexpr FreetypeGlyphImage(FT_Bitmap bitmap): bitmap{bitmap} {}

protected:
	FT_Bitmap bitmap{};
};

static constexpr uint MAX_FREETYPE_SLOTS = Config::envIsLinux ? 4 : 2;
class FontSizeData
{
public:
	constexpr FontSizeData() {}
	constexpr FontSizeData(FontSettings settings): settings{settings} {}
	FontSettings settings{};
	std::array<FT_Size, MAX_FREETYPE_SLOTS> ftSize{};
};

class FreetypeFontSize : public FontSizeData
{
public:
	FreetypeFontSize() {}
	FreetypeFontSize(FontSettings settings): FontSizeData{settings} {}
	~FreetypeFontSize();
	FreetypeFontSize(FreetypeFontSize &&o);
	FreetypeFontSize &operator=(FreetypeFontSize o);
	static void swap(FreetypeFontSize &a, FreetypeFontSize &b);
};

struct FreetypeFaceData
{
	std::error_code openFont(GenericIO file);
	FT_Face face{};
	FT_StreamRec streamRec{};
};

class FreetypeFont
{
public:
	struct GlyphRenderData
	{
		GlyphMetrics metrics{};
		FT_Bitmap bitmap{};
	};

	constexpr FreetypeFont() {}

protected:
	StaticArrayList<FreetypeFaceData, MAX_FREETYPE_SLOTS> f{};
	bool isBold{};

	std::error_code loadIntoNextSlot(GenericIO io);
	std::error_code loadIntoNextSlot(const char *name);
	GlyphRenderData makeGlyphRenderData(int idx, FreetypeFontSize &fontSize, bool keepPixData, std::error_code &ec);
};

using GlyphImageImpl = FreetypeGlyphImage;
using FontImpl = FreetypeFont;
using FontSize = FreetypeFontSize;

}
