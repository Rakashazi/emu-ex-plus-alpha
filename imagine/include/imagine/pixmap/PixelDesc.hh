#pragma once

#include <imagine/engine-globals.h>
#include <imagine/util/typeTraits.hh>
#include <imagine/util/bits.h>
#include <imagine/util/number.h>

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

	template<class T, ENABLE_IF_COND(std::is_integral<T>)>
	constexpr uint build(T r, T g, T b, T a) const
	{
		return (rBits ? ((r & bit_fullMask<uint>(rBits)) << rShift) : 0) |
			(gBits ? ((g & bit_fullMask<uint>(gBits)) << gShift) : 0) |
			(bBits ? ((b & bit_fullMask<uint>(bBits)) << bShift) : 0) |
			(aBits ? ((a & bit_fullMask<uint>(aBits)) << aShift) : 0);
	}

	template<class T, ENABLE_IF_COND(std::is_floating_point<T>)>
	constexpr uint build(T r, T g, T b, T a) const
	{
		return build(IG::scaleDecToBits<uint>(r, rBits), IG::scaleDecToBits<uint>(g, gBits), IG::scaleDecToBits<uint>(b, bBits), IG::scaleDecToBits<uint>(a, aBits));
	}

	static constexpr uint component(uint pixel, uchar shift, uchar bits)
	{
		return (pixel >> shift) & bit_fullMask<uint>(bits);
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
