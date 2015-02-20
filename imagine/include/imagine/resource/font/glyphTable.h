#pragma once

#include <imagine/gfx/Texture.hh>

struct GlyphMetrics
{
	int xSize = 0;
	int ySize = 0;
	int xOffset = 0;
	int yOffset = 0;
	int xAdvance = 0;

	constexpr GlyphMetrics() {}
};

struct GlyphEntry
{
	Gfx::PixmapTexture glyph;
	GlyphMetrics metrics;

	constexpr GlyphEntry() {}
};
