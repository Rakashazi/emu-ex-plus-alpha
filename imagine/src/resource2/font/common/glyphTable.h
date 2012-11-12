#pragma once

struct GlyphMetrics
{
	constexpr GlyphMetrics() { }
	int xSize = 0;
	int ySize = 0;
	int xOffset = 0;
	int yOffset = 0;
	int xAdvance = 0;
};

class ResourceImageGlyph;
struct GlyphEntry
{
	constexpr GlyphEntry() { }
	ResourceImageGlyph *glyph = nullptr;
	GlyphMetrics metrics;
};
