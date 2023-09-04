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
#include <imagine/font/Font.hh>
#include <imagine/gfx/Texture.hh>
#include <imagine/util/container/VMemArray.hh>
#include <string_view>

namespace IG::Gfx
{

constexpr auto glyphSamplerConfig = SamplerConfigs::noMipClamp;

struct GlyphEntry
{
	Texture glyph;
	GlyphMetrics metrics;
};

class GlyphTextureSet
{
public:
	constexpr GlyphTextureSet() = default;
	GlyphTextureSet(Renderer &, Font, FontSettings settings = {});
	FontSettings fontSettings() const;
	bool setFontSettings(Renderer &r, FontSettings set);
	int precache(Renderer &r, std::string_view string);
	int precacheAlphaNum(Renderer &r)
	{
		return precache(r, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
	}
	const GlyphEntry *glyphEntry(Renderer &r, int c, bool allowCache = true);
	GlyphSetMetrics metrics() const { return metrics_; }
	int nominalHeight() const { return metrics().nominalHeight; }
	void freeCaches(uint32_t rangeToFreeBits);
	void freeCaches() { freeCaches(~0); }

private:
	Font font;
	VMemArray<GlyphEntry> glyphTable;
	FontSettings settings;
	FontSize faceSize;
	GlyphSetMetrics metrics_;
	uint32_t usedGlyphTableBits{};

	void calcMetrics(Renderer &r);
	void resetGlyphTable();
	bool cacheChar(Renderer &r, int c, int tableIdx);
};

}
