#pragma once

#include <gfx/GfxBufferImage.hh>

struct GlyphMetrics
{
	constexpr GlyphMetrics() { }
	int xSize = 0;
	int ySize = 0;
	int xOffset = 0;
	int yOffset = 0;
	int xAdvance = 0;
};

namespace Gfx
{

class BufferImage;

}

struct GlyphEntry
{
	constexpr GlyphEntry() { }
	Gfx::BufferImage glyph;
	GlyphMetrics metrics;
};
