#pragma once

struct GlyphMetrics
{
	int xSize;
	int ySize;
	int xOffset;
	int yOffset;
	int xAdvance;
};

class ResourceImageGlyph;
struct GlyphEntry
{
	ResourceImageGlyph *glyph;
	GlyphMetrics metrics;
};
