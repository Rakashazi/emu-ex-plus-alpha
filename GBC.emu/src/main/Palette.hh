#pragma once

#include <cstdint>
#include <span>

namespace EmuEx
{

struct GBPalette
{
	uint32_t bg[4], sp1[4], sp2[4];
};

constexpr size_t gbNumPalettes = 13;

const GBPalette *findGbcTitlePal(char const *title);
std::span<const GBPalette, gbNumPalettes> gbPalettes();
void applyGBPalette();

}
