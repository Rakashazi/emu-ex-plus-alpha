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
#include <imagine/util/bit.hh>
#include <imagine/util/algorithm.h>
#include <imagine/util/utility.h>
#include <concepts>
#include <array>

namespace IG
{

class PixelDesc
{
public:
	const char *name_{};
	int8_t rBits{}, gBits{}, bBits{}, aBits{};
	int8_t rShift{}, gShift{}, bShift{}, aShift{};
	int8_t bytesPerPixel_{};

	constexpr PixelDesc() = default;

	constexpr PixelDesc(int8_t rBits, int8_t gBits, int8_t bBits, int8_t aBits,
		int8_t rShift, int8_t gShift, int8_t bShift, int8_t aShift,
		int8_t bytesPerPixel, const char *name):
		name_{name},
		rBits{rBits}, gBits{gBits}, bBits{bBits}, aBits{aBits},
		rShift{rShift}, gShift{gShift}, bShift{bShift}, aShift{aShift},
		bytesPerPixel_{bytesPerPixel} {}

	constexpr uint32_t build(std::integral auto r_, std::integral auto g_, std::integral auto b_,
		std::integral auto a_) const
	{
		auto r = (uint32_t)r_;
		auto g = (uint32_t)g_;
		auto b = (uint32_t)b_;
		auto a = (uint32_t)a_;
		return (rBits ? ((r & bits<uint32_t>(rBits)) << rShift) : 0) |
			(gBits ? ((g & bits<uint32_t>(gBits)) << gShift) : 0) |
			(bBits ? ((b & bits<uint32_t>(bBits)) << bShift) : 0) |
			(aBits ? ((a & bits<uint32_t>(aBits)) << aShift) : 0);
	}

	constexpr uint32_t build(float r_, float g_, float b_, float a_ = 1.) const
	{
		assumeExpr(r_ >= 0. && r_ <= 1.);
		assumeExpr(g_ >= 0. && g_ <= 1.);
		assumeExpr(b_ >= 0. && b_ <= 1.);
		assumeExpr(a_ >= 0. && a_ <= 1.);
		return build(
			static_cast<uint32_t>(remap(r_, 0.f, 1.f, 0, bits(rBits))),
			static_cast<uint32_t>(remap(g_, 0.f, 1.f, 0, bits(gBits))),
			static_cast<uint32_t>(remap(b_, 0.f, 1.f, 0, bits(bBits))),
			static_cast<uint32_t>(remap(a_, 0.f, 1.f, 0, bits(aBits))));
	}

	static constexpr uint8_t component(uint32_t pixel, uint8_t shift, uint8_t bits_)
	{
		return (pixel >> shift) & bits<uint32_t>(bits_);
	}

	constexpr uint32_t a(uint32_t pixel) const { return component(pixel, aShift, aBits); }
	constexpr uint32_t r(uint32_t pixel) const { return component(pixel, rShift, rBits); }
	constexpr uint32_t g(uint32_t pixel) const { return component(pixel, gShift, gBits); }
	constexpr uint32_t b(uint32_t pixel) const { return component(pixel, bShift, bBits); }

	constexpr std::array<uint8_t, 4> rgba(uint32_t pixel) const
	{
		return {(uint8_t)r(pixel), (uint8_t)g(pixel), (uint8_t)b(pixel), (uint8_t)a(pixel)};
	}

	constexpr float aNorm(uint32_t pixel) const { return a(pixel) / float(1 << aBits); }
	constexpr float rNorm(uint32_t pixel) const { return r(pixel) / float(1 << rBits); }
	constexpr float gNorm(uint32_t pixel) const { return g(pixel) / float(1 << gBits); }
	constexpr float bNorm(uint32_t pixel) const { return b(pixel) / float(1 << bBits); }

	constexpr std::array<float, 4> rgbaNorm(uint32_t pixel) const
	{
		return {rNorm(pixel), gNorm(pixel), bNorm(pixel), aNorm(pixel)};
	}

	constexpr int offsetBytes(int x, int y, int pitch) const
	{
		return (y * pitch) + pixelBytes(x);
	}

	constexpr int pixelBytes(int pixels) const
	{
		return pixels * bytesPerPixel_;
	}

	constexpr int bytesPerPixel() const
	{
		return bytesPerPixel_;
	}

	constexpr int bitsPerPixel() const
	{
		return bytesPerPixel_ * 8;
	}

	constexpr const char *name() const
	{
		return name_;
	}

	constexpr bool isGrayscale() const
	{
		return rBits && !gBits && !bBits;
	}

	constexpr bool isBGROrder() const
	{
		return bShift > rShift;
	}

	constexpr PixelDesc reversed() const
	{
		return {rBits, gBits, bBits, aBits,
			// reverse bit shift values
			int8_t(bitsPerPixel() - rShift - rBits),
			int8_t(bitsPerPixel() - gShift - gBits),
			int8_t(bitsPerPixel() - bShift - bBits),
			int8_t(bitsPerPixel() - aShift - aBits),
			bytesPerPixel_, name_};
	}

	constexpr PixelDesc nativeOrder() const
	{
		if constexpr(std::endian::native == std::endian::little)
		{
			return reversed();
		}
		else
		{
			return *this;
		}
	}
};

enum class PixelFormatId : uint8_t
{
	Unset = 0,
	RGBA8888,
	BGRA8888,
	RGB565,
	RGBA5551,
	RGBA4444,
	I8,
	A8,
	IA88,
	RGB888,
};

constexpr PixelDesc PixelDescUnset		{0, 0, 0, 0, 0,		0, 	0, 	0, 	0, "Unset"};
constexpr PixelDesc PixelDescI8				{8, 0, 0, 0, 0, 	0, 	0, 	0, 	1, "I8"};
constexpr PixelDesc PixelDescA8  			{0, 0, 0, 8, 0, 	0, 	0, 	0, 	1, "A8"};
constexpr PixelDesc PixelDescIA88  		{8, 0, 0, 8, 8, 	0, 	0, 	0, 	2, "IA88"};
constexpr PixelDesc PixelDescRGB565  	{5, 6, 5, 0, 11,	5, 	0, 	0, 	2, "RGB565"};
constexpr PixelDesc PixelDescRGBA5551 {5, 5, 5, 1, 11,	6, 	1, 	0, 	2, "RGBA5551"};
constexpr PixelDesc PixelDescRGBA4444 {4, 4, 4, 4, 12,	8, 	4, 	0, 	2, "RGBA4444"};
constexpr PixelDesc PixelDescRGB888 	{8, 8, 8, 0, 16,	8, 	0, 	0, 	3, "RGB888"};
constexpr PixelDesc PixelDescRGBA8888 {8, 8, 8, 8, 24,	16, 8, 	0, 	4, "RGBA8888"};
constexpr PixelDesc PixelDescBGRA8888 {8, 8, 8, 8, 8, 	16, 24, 0, 	4, "BGRA8888"};
constexpr PixelDesc PixelDescRGBA8888Native = PixelDescRGBA8888.nativeOrder();
constexpr PixelDesc PixelDescBGRA8888Native = PixelDescBGRA8888.nativeOrder();

class PixelFormat
{
public:
	PixelFormatId id{};

	constexpr PixelFormat() = default;
	constexpr PixelFormat(PixelFormatId id): id{id} {}
	constexpr operator PixelFormatId() const { return id; }
	constexpr int offsetBytes(int x, int y, int pitchBytes) const { return desc().offsetBytes(x, y, pitchBytes); }
	constexpr int pixelBytes(int pixels) const { return desc().pixelBytes(pixels); }
	constexpr int bytesPerPixel() const { return desc().bytesPerPixel(); }
	constexpr int bitsPerPixel() const { return bytesPerPixel() * 8; }
	constexpr const char *name() const { return desc().name(); }
	constexpr bool isGrayscale() const { return desc().isGrayscale(); }
	constexpr bool isBGROrder() const { return desc().isBGROrder(); }
	explicit constexpr operator bool() const { return (bool)id; }
	constexpr PixelDesc desc() const { return desc(id); }

	static constexpr PixelDesc desc(PixelFormatId id)
	{
		using enum PixelFormatId;
		switch(id)
		{
			case I8: return PixelDescI8;
			case A8: return PixelDescA8;
			case IA88: return PixelDescIA88;
			case RGB565: return PixelDescRGB565;
			case RGBA5551: return PixelDescRGBA5551;
			case RGBA4444: return PixelDescRGBA4444;
			case RGB888: return PixelDescRGB888;
			case RGBA8888: return PixelDescRGBA8888;
			case BGRA8888: return PixelDescBGRA8888;
			case Unset: break;
		}
		return PixelDescUnset;
	}
};

constexpr PixelFormat PixelFmtUnset{PixelFormatId::Unset};
constexpr PixelFormat PixelFmtI8{PixelFormatId::I8};
constexpr PixelFormat PixelFmtA8{PixelFormatId::A8};
constexpr PixelFormat PixelFmtIA88{PixelFormatId::IA88};
constexpr PixelFormat PixelFmtRGB565{PixelFormatId::RGB565};
constexpr PixelFormat PixelFmtRGBA5551{PixelFormatId::RGBA5551};
constexpr PixelFormat PixelFmtRGBA4444{PixelFormatId::RGBA4444};
constexpr PixelFormat PixelFmtRGB888{PixelFormatId::RGB888};
constexpr PixelFormat PixelFmtRGBA8888{PixelFormatId::RGBA8888};
constexpr PixelFormat PixelFmtBGRA8888{PixelFormatId::BGRA8888};

}
