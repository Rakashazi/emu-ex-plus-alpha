#pragma once

#include <cstddef>
#include <imagine/config/defs.hh>
#include <imagine/util/typeTraits.hh>
#include <imagine/util/bits.h>
#include <imagine/util/math/math.hh>

namespace IG
{

class PixelDesc
{
public:
	const uint rBits = 0, gBits = 0, bBits = 0, aBits = 0;
	const uint rShift = 0, gShift = 0, bShift = 0, aShift = 0;
	const uint bytesPerPixel_ = 0;
	const char *name_{};

	constexpr PixelDesc(uint rBits, uint gBits, uint bBits, uint aBits,
		uint rShift, uint gShift, uint bShift, uint aShift,
		uint bytesPerPixel, const char *name):
		rBits{rBits}, gBits{gBits}, bBits{bBits}, aBits{aBits},
		rShift{rShift}, gShift{gShift}, bShift{bShift}, aShift{aShift},
		bytesPerPixel_{bytesPerPixel}, name_{name}
	{}

	template<class T>
	constexpr uint build(T r_, T g_, T b_, T a_) const
	{
		uint r = 0, g = 0, b = 0, a = 0;
		if constexpr(std::is_floating_point<T>::value)
		{
			r = IG::scaleDecToBits<uint>(r_, rBits);
			g = IG::scaleDecToBits<uint>(g_, gBits);
			b = IG::scaleDecToBits<uint>(b_, bBits);
			a = IG::scaleDecToBits<uint>(a_, aBits);
		}
		else
		{
			r = (uint)r_;
			g = (uint)g_;
			b = (uint)b_;
			a = (uint)a_;
		}
		return (rBits ? ((r & makeFullBits<uint>(rBits)) << rShift) : 0) |
			(gBits ? ((g & makeFullBits<uint>(gBits)) << gShift) : 0) |
			(bBits ? ((b & makeFullBits<uint>(bBits)) << bShift) : 0) |
			(aBits ? ((a & makeFullBits<uint>(aBits)) << aShift) : 0);
	}

	static constexpr uint component(uint pixel, uchar shift, uchar bits)
	{
		return (pixel >> shift) & makeFullBits<uint>(bits);
	}

	constexpr uint a(uint pixel) const { return component(pixel, aShift, aBits); }
	constexpr uint r(uint pixel) const { return component(pixel, rShift, rBits); }
	constexpr uint g(uint pixel) const { return component(pixel, gShift, gBits); }
	constexpr uint b(uint pixel) const { return component(pixel, bShift, bBits); }

	constexpr size_t offsetBytes(int x, int y, uint pitch)
	{
		return (y * pitch) + pixelBytes(x);
	}

	constexpr size_t pixelBytes(int pixels) const
	{
		return pixels * bytesPerPixel_;
	}

	constexpr uint bytesPerPixel() const
	{
		return bytesPerPixel_;
	}

	constexpr uint bitsPerPixel() const
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
