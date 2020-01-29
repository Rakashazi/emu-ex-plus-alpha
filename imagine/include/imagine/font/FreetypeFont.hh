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
	constexpr FreetypeGlyphImage() {}
	FreetypeGlyphImage(FT_Bitmap bitmap);
	FreetypeGlyphImage(FreetypeGlyphImage &&o);
	FreetypeGlyphImage &operator=(FreetypeGlyphImage &&o);
	~FreetypeGlyphImage();

protected:
	FT_Bitmap bitmap{};
};

static constexpr uint32_t MAX_FREETYPE_SLOTS = Config::envIsLinux ? 4 : 2;

class FreetypeFontSize
{
public:
	using FTSizeArray = std::array<FT_Size, MAX_FREETYPE_SLOTS>;

	constexpr FreetypeFontSize() {}
	FreetypeFontSize(FontSettings settings);
	FreetypeFontSize(FreetypeFontSize &&o);
	FreetypeFontSize &operator=(FreetypeFontSize &&o);
	~FreetypeFontSize();

	FTSizeArray &sizeArray();
	FontSettings fontSettings() const;

protected:
	FontSettings settings{};
	FTSizeArray ftSize{};

	void deinit();
};

struct FreetypeFaceData
{
	std::errc openFont(GenericIO file);
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
	FreetypeFont(FreetypeFont &&o);
	FreetypeFont &operator=(FreetypeFont &&o);
	~FreetypeFont();

protected:
	StaticArrayList<FreetypeFaceData, MAX_FREETYPE_SLOTS> f{};
	bool isBold{};

	std::errc loadIntoNextSlot(GenericIO io);
	std::errc loadIntoNextSlot(const char *name);
	GlyphRenderData makeGlyphRenderData(int idx, FreetypeFontSize &fontSize, bool keepPixData, std::errc &ec);
	void deinit();
};

using GlyphImageImpl = FreetypeGlyphImage;
using FontImpl = FreetypeFont;
using FontSize = FreetypeFontSize;

}
