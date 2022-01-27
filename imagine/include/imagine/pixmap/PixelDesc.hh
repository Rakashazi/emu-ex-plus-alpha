#pragma once

#include <imagine/config/defs.hh>
#include <imagine/util/bitset.hh>
#include <imagine/util/math/math.hh>
#include <imagine/util/concepts.hh>
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
		bytesPerPixel_{bytesPerPixel}
	{}

	constexpr uint32_t build(IG::floating_point auto r_, IG::floating_point auto g_, IG::floating_point auto b_,
		IG::floating_point auto a_) const
	{
		return build(IG::clampFromFloat<uint32_t>(r_, rBits),
			IG::clampFromFloat<uint32_t>(g_, gBits),
			IG::clampFromFloat<uint32_t>(b_, bBits),
			IG::clampFromFloat<uint32_t>(a_, aBits));
	}

	constexpr uint32_t build(IG::integral auto r_, IG::integral auto g_, IG::integral auto b_,
		IG::integral auto a_) const
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

	constexpr size_t offsetBytes(int x, int y, uint32_t pitch) const
	{
		return (y * pitch) + pixelBytes(x);
	}

	constexpr size_t pixelBytes(int pixels) const
	{
		return pixels * bytesPerPixel_;
	}

	constexpr uint8_t bytesPerPixel() const
	{
		return bytesPerPixel_;
	}

	constexpr uint8_t bitsPerPixel() const
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
