#pragma once

#include <cstdint>

namespace EmuEx
{

struct GBPalette
{
	uint32_t bg[4], sp1[4], sp2[4];
};

GBPalette const *findGbcTitlePal(char const *title);

}
