#pragma once

#include <imagine/config/defs.hh>
#include <imagine/util/bits.h>
#include <imagine/util/math/math.hh>
#include <type_traits>

namespace IG
{

class PixelDesc
{
public:
	const uint32_t rBits = 0, gBits = 0, bBits = 0, aBits = 0;
	const uint32_t rShift = 0, gShift = 0, bShift = 0, aShift = 0;
	const uint32_t bytesPerPixel_ = 0;
	const char *name_{};

	constexpr PixelDesc(uint32_t rBits, uint32_t gBits, uint32_t bBits, uint32_t aBits,
		uint32_t rShift, uint32_t gShift, uint32_t bShift, uint32_t aShift,
		uint32_t bytesPerPixel, const char *name):
		rBits{rBits}, gBits{gBits}, bBits{bBits}, aBits{aBits},
		rShift{rShift}, gShift{gShift}, bShift{bShift}, aShift{aShift},
		bytesPerPixel_{bytesPerPixel}, name_{name}
	{}

	template<class T>
	constexpr uint32_t build(T r_, T g_, T b_, T a_) const
	{
		uint32_t r = 0, g = 0, b = 0, a = 0;
		if constexpr(std::is_floating_point_v<T>)
		{
			r = IG::scaleDecToBits<uint32_t>(r_, rBits);
			g = IG::scaleDecToBits<uint32_t>(g_, gBits);
			b = IG::scaleDecToBits<uint32_t>(b_, bBits);
			a = IG::scaleDecToBits<uint32_t>(a_, aBits);
		}
		else
		{
			r = (uint32_t)r_;
			g = (uint32_t)g_;
			b = (uint32_t)b_;
			a = (uint32_t)a_;
		}
		return (rBits ? ((r & makeFullBits<uint32_t>(rBits)) << rShift) : 0) |
			(gBits ? ((g & makeFullBits<uint32_t>(gBits)) << gShift) : 0) |
			(bBits ? ((b & makeFullBits<uint32_t>(bBits)) << bShift) : 0) |
			(aBits ? ((a & makeFullBits<uint32_t>(aBits)) << aShift) : 0);
	}

	static constexpr uint32_t component(uint32_t pixel, uint8_t shift, uint8_t bits)
	{
		return (pixel >> shift) & makeFullBits<uint32_t>(bits);
	}

	constexpr uint32_t a(uint32_t pixel) const { return component(pixel, aShift, aBits); }
	constexpr uint32_t r(uint32_t pixel) const { return component(pixel, rShift, rBits); }
	constexpr uint32_t g(uint32_t pixel) const { return component(pixel, gShift, gBits); }
	constexpr uint32_t b(uint32_t pixel) const { return component(pixel, bShift, bBits); }

	constexpr size_t offsetBytes(int x, int y, uint32_t pitch)
	{
		return (y * pitch) + pixelBytes(x);
	}

	constexpr size_t pixelBytes(int pixels) const
	{
		return pixels * bytesPerPixel_;
	}

	constexpr uint32_t bytesPerPixel() const
	{
		return bytesPerPixel_;
	}

	constexpr uint32_t bitsPerPixel() const
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
};

}
