#pragma once

namespace IG
{

struct GlyphMetrics
{
	int xSize = 0;
	int ySize = 0;
	int xOffset = 0;
	int yOffset = 0;
	int xAdvance = 0;

	constexpr GlyphMetrics() {}
};

}
