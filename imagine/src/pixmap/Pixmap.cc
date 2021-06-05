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

#define LOGTAG "Pixmap"
#include <imagine/pixmap/Pixmap.hh>
#include <imagine/pixmap/MemPixmap.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/utility.h>
#include <imagine/util/algorithm.h>
#include <imagine/util/container/array.hh>
#include <cstring>

namespace IG
{

void Pixmap::write(Pixmap pixmap)
{
	assumeExpr(format() == pixmap.format());
	if(w() == pixmap.w() && !isPadded() && !pixmap.isPadded())
	{
		// whole block
		//logDMsg("copying whole block");
		memcpy(data_, pixmap.data_, pixmap.unpaddedBytes());
	}
	else
	{
		// line at a time
		auto srcData = pixmap.data();
		auto destData = data();
		auto destPitch = pitch;
		uint32_t lineBytes = format().pixelBytes(pixmap.w());
		iterateTimes(pixmap.h(), i)
		{
			memcpy(destData, srcData, lineBytes);
			srcData += pixmap.pitch;
			destData += destPitch;
		}
	}
}

void Pixmap::write(Pixmap pixmap, WP destPos)
{
	subView(destPos, size() - destPos).write(pixmap);
}

static void invalidFormatConversion(Pixmap dest, Pixmap src)
{
	logErr("unimplemented conversion:%s -> %s", src.format().name(), dest.format().name());
}

template <bool BGR_SWAP = false>
static void convertRGB888ToRGBX8888(Pixmap dest, Pixmap src)
{
	dest.writeTransformedDirect<ByteArray<3>, uint32_t>(
		[](auto p)
		{
			if constexpr(BGR_SWAP) { std::swap(p[0], p[2]); }
			return p[0] << 16 |
					p[1] << 8 |
					p[2];
		}, src);
}

static void convertRGB888ToBGRX8888(Pixmap dest, Pixmap src)
{
	convertRGB888ToRGBX8888<true>(dest, src);
}

template <bool BGR_SWAP = false>
static void convertRGB565ToRGBX8888(Pixmap dest, Pixmap src)
{
	dest.writeTransformed(
		[](uint16_t p)
		{
			unsigned b = p       & 0x1F;
			unsigned g = p >>  5 & 0x3F;
			unsigned r = p >> 11 & 0x1F;
			if constexpr(BGR_SWAP) { std::swap(r, b); }
			return ((b * 255 + 15) / 31) << 16 |
					((g * 255 + 31) / 63) << 8 |
					((r * 255 + 15) / 31);
		}, src);
}

static void convertRGB565ToBGRX8888(Pixmap dest, Pixmap src)
{
	return convertRGB565ToRGBX8888<true>(dest, src);
}

template <bool BGR_SWAP = false>
static void convertRGBX8888ToRGB888(Pixmap dest, Pixmap src)
{
	dest.writeTransformedDirect<uint32_t, ByteArray<3>>(
		[](auto p)
		{
			unsigned r = p       & 0xFF;
			unsigned g = p >>  8 & 0xFF;
			unsigned b = p >> 16 & 0xFF;
			if constexpr(BGR_SWAP) { std::swap(r, b); }
			return ByteArray<3>
				{
					(uint8_t)r,
					(uint8_t)g,
					(uint8_t)b
				};
		}, src);
}

static void convertBGRX8888ToRGB888(Pixmap dest, Pixmap src)
{
	convertRGBX8888ToRGB888<true>(dest, src);
}

static void convertRGB565ToRGB888(Pixmap dest, Pixmap src)
{
	dest.writeTransformedDirect<uint16_t, ByteArray<3>>(
		[](auto p)
		{
			unsigned b = p       & 0x1F;
			unsigned g = p >>  5 & 0x3F;
			unsigned r = p >> 11 & 0x1F;
			return ByteArray<3>
				{
					uint8_t((r * 255 + 15) / 31),
					uint8_t((g * 255 + 31) / 63),
					uint8_t((b * 255 + 15) / 31)
				};
		}, src);
}

static void convertRGB888ToRGB565(Pixmap dest, Pixmap src)
{
	dest.writeTransformedDirect<ByteArray<3>, uint16_t>(
		[](ByteArray<3> p)
		{
			unsigned r = p[0];
			unsigned g = p[1];
			unsigned b = p[2];
			return ((r * (31 * 2) + 255) / (255 * 2)) << 11 |
					((g * 63 + 127) / 255) << 5 |
					((b * 31 + 127) / 255);
		}, src);
}

template <bool BGR_SWAP = false>
static void convertRGBX8888ToRGB565(Pixmap dest, Pixmap src)
{
	dest.writeTransformed(
		[](uint32_t p)
		{
			unsigned r = p       & 0xFF;
			unsigned g = p >>  8 & 0xFF;
			unsigned b = p >> 16 & 0xFF;
			if constexpr(BGR_SWAP) { std::swap(r, b); }
			return ((r * (31 * 2) + 255) / (255 * 2)) << 11 |
					((g * 63 + 127) / 255) << 5 |
					((b * 31 + 127) / 255);
		}, src);
}

static void convertRGBA8888ToBGRA8888(Pixmap dest, Pixmap src)
{
	dest.writeTransformed(
		[](uint32_t p)
		{
			return (p & 0xFF000000) | ((p & 0xFF0000) >> 16) | (p & 0x00FF00) | ((p & 0x0000FF) << 16);
		}, src);
}

static void convertBGRX8888ToRGB565(Pixmap dest, Pixmap src)
{
	return convertRGBX8888ToRGB565<true>(dest, src);
}

void Pixmap::writeConverted(Pixmap pixmap)
{
	if(format() == pixmap.format())
	{
		write(pixmap);
		return;
	}
	auto srcFormatID = pixmap.format().id();
	switch(format().id())
	{
		case PIXEL_RGBA8888:
			switch(srcFormatID)
			{
				case PIXEL_BGRA8888: return convertRGBA8888ToBGRA8888(*this, pixmap);
				case PIXEL_RGB565: return convertRGB565ToRGBX8888(*this, pixmap);
				case PIXEL_RGB888: return convertRGB888ToRGBX8888(*this, pixmap);
				default: return invalidFormatConversion(*this, pixmap);
			}
		case PIXEL_BGRA8888:
			switch(srcFormatID)
			{
				case PIXEL_RGBA8888: return convertRGBA8888ToBGRA8888(*this, pixmap);
				case PIXEL_RGB565: return convertRGB565ToBGRX8888(*this, pixmap);
				case PIXEL_RGB888: return convertRGB888ToBGRX8888(*this, pixmap);
				default: return invalidFormatConversion(*this, pixmap);
			}
		case PIXEL_RGB888:
			switch(srcFormatID)
			{
				case PIXEL_BGRA8888: return convertBGRX8888ToRGB888(*this, pixmap);
				case PIXEL_RGBA8888: return convertRGBX8888ToRGB888(*this, pixmap);
				case PIXEL_RGB565: return convertRGB565ToRGB888(*this, pixmap);
				default: return invalidFormatConversion(*this, pixmap);
			}
		case PIXEL_RGB565:
			switch(srcFormatID)
			{
				case PIXEL_RGBA8888: return convertRGBX8888ToRGB565(*this, pixmap);
				case PIXEL_BGRA8888: return convertBGRX8888ToRGB565(*this, pixmap);
				case PIXEL_RGB888: return convertRGB888ToRGB565(*this, pixmap);
				default: return invalidFormatConversion(*this, pixmap);
			}
		default:
			return invalidFormatConversion(*this, pixmap);
	}
}

void Pixmap::writeConverted(Pixmap pixmap, WP destPos)
{
	subView(destPos, size() - destPos).writeConverted(pixmap);
}

void Pixmap::clear(WP pos, WP size)
{
	char *destData = pixel(pos);
	if(!isPadded() && (int)w() == size.x)
	{
		std::fill_n(destData, format().pixelBytes(size.x * size.y), 0);
	}
	else
	{
		uint32_t lineBytes = format().pixelBytes(size.x);
		iterateTimes(size.y, i)
		{
			std::fill_n(destData, lineBytes, 0);
			destData += pitch;
		}
	}
}

void Pixmap::clear()
{
	clear({}, {(int)w(), (int)h()});
}

MemPixmap::MemPixmap(PixmapDesc desc):
	PixmapDesc{desc},
	buffer{new uint8_t[desc.bytes()]}
{
	//logDMsg("allocated memory pixmap data:%p", data);
}

MemPixmap::operator bool() const
{
	return (bool)buffer;
}

Pixmap MemPixmap::view() const
{
	return Pixmap{static_cast<PixmapDesc>(*this), buffer.get()};
}

Pixmap MemPixmap::subView(WP pos, WP size) const
{
	return view().subView(pos, size);
}

}
