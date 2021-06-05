#pragma once

#include <imagine/config/defs.hh>
#include <imagine/util/bitset.hh>
#include <imagine/util/math/math.hh>
#include <type_traits>
#include <bit>

namespace IG
{

class PixelDesc
{
public:
	const char *name_{};
	uint8_t rBits{}, gBits{}, bBits{}, aBits{};
	uint8_t rShift{}, gShift{}, bShift{}, aShift{};
	uint8_t bytesPerPixel_{};

	constexpr PixelDesc() {}

	constexpr PixelDesc(uint8_t rBits, uint8_t gBits, uint8_t bBits, uint8_t aBits,
		uint8_t rShift, uint8_t gShift, uint8_t bShift, uint8_t aShift,
		uint8_t bytesPerPixel, const char *name):
		name_{name},
		rBits{rBits}, gBits{gBits}, bBits{bBits}, aBits{aBits},
		rShift{rShift}, gShift{gShift}, bShift{bShift}, aShift{aShift},
		bytesPerPixel_{bytesPerPixel}
	{}

	template<class T>
	constexpr uint32_t build(T r_, T g_, T b_, T a_) const
	{
		uint32_t r = 0, g = 0, b = 0, a = 0;
		if constexpr(std::is_floating_point_v<T>)
		{
			r = IG::clampFromFloat<uint32_t>(r_, rBits);
			g = IG::clampFromFloat<uint32_t>(g_, gBits);
			b = IG::clampFromFloat<uint32_t>(b_, bBits);
			a = IG::clampFromFloat<uint32_t>(a_, aBits);
		}
		else
		{
			r = (uint32_t)r_;
			g = (uint32_t)g_;
			b = (uint32_t)b_;
			a = (uint32_t)a_;
		}
		return (rBits ? ((r & bits<uint32_t>(rBits)) << rShift) : 0) |
			(gBits ? ((g & bits<uint32_t>(gBits)) << gShift) : 0) |
			(bBits ? ((b & bits<uint32_t>(bBits)) << bShift) : 0) |
			(aBits ? ((a & bits<uint32_t>(aBits)) << aShift) : 0);
	}

	static constexpr uint32_t component(uint32_t pixel, uint8_t shift, uint8_t bits_)
	{
		return (pixel >> shift) & bits<uint32_t>(bits_);
	}

	constexpr uint32_t a(uint32_t pixel) const { return component(pixel, aShift, aBits); }
	constexpr uint32_t r(uint32_t pixel) const { return component(pixel, rShift, rBits); }
	constexpr uint32_t g(uint32_t pixel) const { return component(pixel, gShift, gBits); }
	constexpr uint32_t b(uint32_t pixel) const { return component(pixel, bShift, bBits); }

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
