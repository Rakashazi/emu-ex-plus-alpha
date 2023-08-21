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
#include <bit>
#include <array>

namespace IG
{

class PixelDesc
{
public:
	const char *name_{};
	uint8_t rBits{}, gBits{}, bBits{}, aBits{};
	uint8_t rShift{}, gShift{}, bShift{}, aShift{};
	uint8_t bytesPerPixel_{};

	constexpr PixelDesc() = default;

	constexpr PixelDesc(uint8_t rBits, uint8_t gBits, uint8_t bBits, uint8_t aBits,
		uint8_t rShift, uint8_t gShift, uint8_t bShift, uint8_t aShift,
		uint8_t bytesPerPixel, const char *name):
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
			(uint8_t)(bitsPerPixel() - rShift - rBits),
			(uint8_t)(bitsPerPixel() - gShift - gBits),
			(uint8_t)(bitsPerPixel() - bShift - bBits),
			(uint8_t)(bitsPerPixel() - aShift - aBits),
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

}
