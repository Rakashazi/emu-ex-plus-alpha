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
#include <imagine/pixmap/PixelDesc.hh>

namespace IG
{

enum PixelFormatID : uint8_t
{
	PIXEL_NONE = 0,
	PIXEL_RGBA8888,
	PIXEL_BGRA8888,
	PIXEL_RGB565,
	PIXEL_RGBA5551,
	PIXEL_RGBA4444,
	PIXEL_I8,
	PIXEL_A8,
	PIXEL_IA88,
	PIXEL_RGB888,
	PIXEL_END,
	PIXEL_MAX = PIXEL_END - 1
};

constexpr PixelDesc PIXEL_DESC_NONE			{0, 0, 0, 0, 0,		0, 	0, 	0, 	0, "None"};
constexpr PixelDesc PIXEL_DESC_I8				{8, 0, 0, 0, 0, 	0, 	0, 	0, 	1, "I8"};
constexpr PixelDesc PIXEL_DESC_A8  			{0, 0, 0, 8, 0, 	0, 	0, 	0, 	1, "A8"};
constexpr PixelDesc PIXEL_DESC_IA88  		{8, 0, 0, 8, 8, 	0, 	0, 	0, 	2, "IA88"};
constexpr PixelDesc PIXEL_DESC_RGB565  	{5, 6, 5, 0, 11,	5, 	0, 	0, 	2, "RGB565"};
constexpr PixelDesc PIXEL_DESC_RGBA5551 	{5, 5, 5, 1, 11,	6, 	1, 	0, 	2, "RGBA5551"};
constexpr PixelDesc PIXEL_DESC_RGBA4444 	{4, 4, 4, 4, 12,	8, 	4, 	0, 	2, "RGBA4444"};
constexpr PixelDesc PIXEL_DESC_RGB888 		{8, 8, 8, 0, 16,	8, 	0, 	0, 	3, "RGB888"};
constexpr PixelDesc PIXEL_DESC_RGBA8888 	{8, 8, 8, 8, 24,	16, 8, 	0, 	4, "RGBA8888"};
constexpr PixelDesc PIXEL_DESC_BGRA8888 	{8, 8, 8, 8, 8, 	16, 24, 0, 	4, "BGRA8888"};
constexpr PixelDesc PIXEL_DESC_RGBA8888_NATIVE = PIXEL_DESC_RGBA8888.nativeOrder();

class PixelFormat
{
public:
	PixelFormatID id{PIXEL_NONE};

	constexpr PixelFormat() = default;
	constexpr PixelFormat(PixelFormatID id): id{id} {}
	constexpr operator PixelFormatID() const { return id; }
	constexpr int offsetBytes(int x, int y, int pitchBytes) const { return desc().offsetBytes(x, y, pitchBytes); }
	constexpr int pixelBytes(int pixels) const { return desc().pixelBytes(pixels); }
	constexpr int bytesPerPixel() const { return desc().bytesPerPixel(); }
	constexpr int bitsPerPixel() const { return bytesPerPixel() * 8; }
	constexpr const char *name() const { return desc().name(); }
	constexpr bool isGrayscale() const { return desc().isGrayscale(); }
	constexpr bool isBGROrder() const { return desc().isBGROrder(); }
	explicit constexpr operator bool() const { return (bool)id; }
	constexpr PixelDesc desc() const { return desc(id); }

	static constexpr PixelDesc desc(PixelFormatID id)
	{
		switch(id)
		{
			case PIXEL_I8: return PIXEL_DESC_I8;
			case PIXEL_A8: return PIXEL_DESC_A8;
			case PIXEL_IA88: return PIXEL_DESC_IA88;
			case PIXEL_RGB565: return PIXEL_DESC_RGB565;
			case PIXEL_RGBA5551: return PIXEL_DESC_RGBA5551;
			case PIXEL_RGBA4444: return PIXEL_DESC_RGBA4444;
			case PIXEL_RGB888: return PIXEL_DESC_RGB888;
			case PIXEL_RGBA8888: return PIXEL_DESC_RGBA8888;
			case PIXEL_BGRA8888: return PIXEL_DESC_BGRA8888;
			case PIXEL_NONE: break;
			case PIXEL_END: break;
		}
		return PIXEL_DESC_NONE;
	}
};

constexpr PixelFormat PIXEL_FMT_NONE{PIXEL_NONE};
constexpr PixelFormat PIXEL_FMT_I8{PIXEL_I8};
constexpr PixelFormat PIXEL_FMT_A8{PIXEL_A8};
constexpr PixelFormat PIXEL_FMT_IA88{PIXEL_IA88};
constexpr PixelFormat PIXEL_FMT_RGB565{PIXEL_RGB565};
constexpr PixelFormat PIXEL_FMT_RGBA5551{PIXEL_RGBA5551};
constexpr PixelFormat PIXEL_FMT_RGBA4444{PIXEL_RGBA4444};
constexpr PixelFormat PIXEL_FMT_RGB888{PIXEL_RGB888};
constexpr PixelFormat PIXEL_FMT_RGBA8888{PIXEL_RGBA8888};
constexpr PixelFormat PIXEL_FMT_BGRA8888{PIXEL_BGRA8888};

}
