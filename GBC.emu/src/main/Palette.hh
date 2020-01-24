#pragma once

#include <cstdint>

struct GBPalette
{
	uint32_t bg[4], sp1[4], sp2[4];
};

GBPalette const *findGbcTitlePal(char const *title);
