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

#include <array>
#include <cstdint>

namespace IG
{

using RGBTripleArray = std::array<unsigned char, 3>;

RGBTripleArray transformRGB565ToRGB888(uint16_t p)
{
	unsigned b = p       & 0x1F;
	unsigned g = p >>  5 & 0x3F;
	unsigned r = p >> 11 & 0x1F;
	return RGBTripleArray
		{
			uint8_t((r * 255 + 15) / 31),
			uint8_t((g * 255 + 31) / 63),
			uint8_t((b * 255 + 15) / 31)
		};
}

uint16_t transformRGB888ToRGB565(RGBTripleArray p)
{
	unsigned r = p[0];
	unsigned g = p[1];
	unsigned b = p[2];
	return ((r * (31 * 2) + 255) / (255 * 2)) << 11 |
			((g * 63 + 127) / 255) << 5 |
			((b * 31 + 127) / 255);
}

uint32_t transformRGBA8888ToBGRA8888(uint32_t p)
{
	return (p & 0xFF000000) | ((p & 0xFF0000) >> 16) | (p & 0x00FF00) | ((p & 0x0000FF) << 16);
}

template <bool BGR_SWAP = false>
static uint16_t transformRGBX8888ToRGB565Impl(uint32_t p)
{
	unsigned r = p       & 0xFF;
	unsigned g = p >>  8 & 0xFF;
	unsigned b = p >> 16 & 0xFF;
	if constexpr(BGR_SWAP) { std::swap(r, b); }
	return ((r * (31 * 2) + 255) / (255 * 2)) << 11 |
			((g * 63 + 127) / 255) << 5 |
			((b * 31 + 127) / 255);
}

uint16_t transformRGBX8888ToRGB565(uint32_t p) { return transformRGBX8888ToRGB565Impl(p); }
uint16_t transformBGRX8888ToRGB565(uint32_t p) { return transformRGBX8888ToRGB565Impl<true>(p); }

template <bool BGR_SWAP = false>
static RGBTripleArray transformRGBX8888ToRGB888Impl(uint32_t p)
{
	unsigned r = p       & 0xFF;
	unsigned g = p >>  8 & 0xFF;
	unsigned b = p >> 16 & 0xFF;
	if constexpr(BGR_SWAP) { std::swap(r, b); }
	return RGBTripleArray
		{
			(uint8_t)r,
			(uint8_t)g,
			(uint8_t)b
		};
}

RGBTripleArray transformRGBX8888ToRGB888(uint32_t p) { return transformRGBX8888ToRGB888Impl(p); }
RGBTripleArray transformBGRX8888ToRGB888(uint32_t p) { return transformRGBX8888ToRGB888Impl<true>(p); }

template <bool BGR_SWAP = false>
static uint32_t transformRGB565ToRGBX8888Impl(uint16_t p)
{
	unsigned b = p       & 0x1F;
	unsigned g = p >>  5 & 0x3F;
	unsigned r = p >> 11 & 0x1F;
	if constexpr(BGR_SWAP) { std::swap(r, b); }
	return ((b * 255 + 15) / 31) << 16 |
			((g * 255 + 31) / 63) << 8 |
			((r * 255 + 15) / 31);
}

uint32_t transformRGB565ToRGBX8888(uint16_t p) { return transformRGB565ToRGBX8888Impl(p); }
uint32_t transformRGB565ToBGRX8888(uint16_t p) { return transformRGB565ToRGBX8888Impl<true>(p); }

template <bool BGR_SWAP = false>
static uint32_t transformRGB888ToRGBX8888Impl(RGBTripleArray p)
{
	if constexpr(BGR_SWAP) { std::swap(p[0], p[2]); }
	return p[0] << 16 |
			p[1] << 8 |
			p[2];
}

uint32_t transformRGB888ToRGBX8888(RGBTripleArray p) { return transformRGB888ToRGBX8888Impl(p); }
uint32_t transformRGB888ToBGRX8888(RGBTripleArray p) { return transformRGB888ToRGBX8888Impl<true>(p); }

}
