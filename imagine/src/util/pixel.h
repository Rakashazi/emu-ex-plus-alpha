#pragma once

#include <logger/interface.h>

#include <util/number.h>
#include <util/bits.h>
#include <assert.h>
#include <util/cLang.h>
#include <util/branch.h>

class PixelFormatDesc
{
public:
	uchar rShift, gShift, bShift, aShift;
	uchar rBits, gBits, bBits, aBits;
	uchar bitsPerPixel, bytesPerPixel;
	uchar bgrOrder;
	const char *name;
	ushort id;

	constexpr uint build(uint r, uint g, uint b, uint a) const
	{
		//uint pixel = 0;
		return (rBits ? ((r & bit_fullMask<uint>(rBits)) << rShift) : 0) |
			(gBits ? ((g & bit_fullMask<uint>(gBits)) << gShift) : 0) |
			(bBits ? ((b & bit_fullMask<uint>(bBits)) << bShift) : 0) |
			(aBits ? ((a & bit_fullMask<uint>(aBits)) << aShift) : 0);
		/*if(rBits) pixel |= (r & bit_fullMask<uint>(rBits)) << rShift;
		if(gBits) pixel |= (g & bit_fullMask<uint>(gBits)) << gShift;
		if(bBits) pixel |= (b & bit_fullMask<uint>(bBits)) << bShift;
		if(aBits) pixel |= (a & bit_fullMask<uint>(aBits)) << aShift;
		return pixel;*/
	}

	constexpr uint build(int r, int g, int b, int a) const
	{
		return build((uint)r, (uint)g, (uint)b, (uint)a);
	}

	constexpr uint build(uchar r, uchar g, uchar b, uchar a) const
	{
		return build((uint)r, (uint)g, (uint)b, (uint)a);
	}

	constexpr uint build(float r, float g, float b, float a) const
	{
		return build(IG::scaleDecToBits<uint>(r, rBits), IG::scaleDecToBits<uint>(g, gBits), IG::scaleDecToBits<uint>(b, bBits), IG::scaleDecToBits<uint>(a, aBits));
	}

	constexpr uint build(double r, double g, double b, double a) const
	{
		return build(IG::scaleDecToBits<uint>(r, rBits), IG::scaleDecToBits<uint>(g, gBits), IG::scaleDecToBits<uint>(b, bBits), IG::scaleDecToBits<uint>(a, aBits));
	}

	static uint component(uint pixel, uchar shift, uchar bits) { return (pixel >> shift) & bit_fullMask<uint>(bits); }
	uint a(uint pixel) const { return component(pixel, aShift, aBits); }
	uint r(uint pixel) const { return component(pixel, rShift, rBits); }
	uint g(uint pixel) const { return component(pixel, gShift, gBits); }
	uint b(uint pixel) const { return component(pixel, bShift, bBits); }

	size_t offsetBytes(int x, int y, int pitch) const
	{
		return (y * pitch) + (x * (bytesPerPixel));
	}

	bool isGrayscale() const
	{
		return (rBits == gBits && gBits == bBits) && (rShift == gShift && gShift == bShift);
	}

	bool isBGROrder() const
	{
		return bgrOrder;
	}
};

// Unique format IDs
enum { PIXEL_UNKNOWN, PIXEL_RGB888, PIXEL_RGBA8888, PIXEL_BGR888, PIXEL_BGRA8888,
	PIXEL_RGB565, PIXEL_ARGB1555, PIXEL_ABGR1555, PIXEL_XBGR1555, PIXEL_I8,
	PIXEL_ARGB8888, PIXEL_ABGR8888, PIXEL_IA53, PIXEL_AI35, PIXEL_IA88,
	PIXEL_ARGB4444, PIXEL_BGRA4444, PIXEL_END } ;

static constexpr PixelFormatDesc PixelFormatRGB888 =
{
	16, 8, 0, 0, // shifts
	8, 8, 8, 0, // bits
	24, 3, 0, "RGB888", PIXEL_RGB888
};
static constexpr PixelFormatDesc PixelFormatRGBA8888 =
{
	24, 16, 8, 0, // shifts
	8, 8, 8, 8, // bits
	32, 4, 0, "RGBA8888", PIXEL_RGBA8888
};
static constexpr PixelFormatDesc PixelFormatARGB8888 =
{
	16, 8, 0, 24, // shifts
	8, 8, 8, 8, // bits
	32, 4, 0, "ARGB8888", PIXEL_ARGB8888
};
static constexpr PixelFormatDesc PixelFormatBGR888 =
{
	0, 8, 16, 0, // shifts
	8, 8, 8, 0, // bits
	24, 3, 1, "BGR888", PIXEL_BGR888
};
static constexpr PixelFormatDesc PixelFormatBGRA8888 =
{
	8, 16, 24, 0, // shifts
	8, 8, 8, 8, // bits
	32, 4, 1, "BGRA8888", PIXEL_BGRA8888
};
static constexpr PixelFormatDesc PixelFormatABGR8888 =
{
	0, 8, 16, 24, // shifts
	8, 8, 8, 8, // bits
	32, 4, 1, "ABGR8888", PIXEL_ABGR8888
};
static constexpr PixelFormatDesc PixelFormatRGB565 =
{
	11, 5, 0, 0, // shifts
	5, 6, 5, 0, // bits
	16, 2, 0, "RGB565", PIXEL_RGB565
};
static constexpr PixelFormatDesc PixelFormatARGB1555 =
{
	10, 5, 0, 15, // shifts
	5, 5, 5, 1, // bits
	16, 2, 0, "ARGB1555", PIXEL_ARGB1555
};
static constexpr PixelFormatDesc PixelFormatABGR1555 =
{
	10, 5, 0, 15, // shifts
	5, 5, 5, 1, // bits
	16, 2, 1, "ABGR1555", PIXEL_ABGR1555
};
static constexpr PixelFormatDesc PixelFormatARGB4444 =
{
	8, 4, 0, 12, // shifts
	4, 4, 4, 4, // bits
	16, 2, 0, "ARGB4444", PIXEL_ARGB4444
};
static constexpr PixelFormatDesc PixelFormatBGRA4444 =
{
	4, 8, 12, 0, // shifts
	4, 4, 4, 4, // bits
	16, 2, 0, "BGRA4444", PIXEL_BGRA4444
};
/*static constexpr PixelFormatDesc PixelFormat1555XRGB = { 10, 5, 0, 0,
	5, 5, 5, 0,
	16, 2, 0, "XRGB1555", 0 };
static constexpr PixelFormatDesc PixelFormat1555XBGR = { 0, 5, 10, 0,
	5, 5, 5, 0,
	16, 2, 1, "XBGR1555", 0 };
static constexpr PixelFormatDesc PixelFormat1555ABGR = { 0, 5, 10, 15,
	5, 5, 5, 1,
	16, 2, 1, "ABGR1555", 0 };
static constexpr PixelFormatDesc PixelFormat5551RGBA = { 1, 6, 11, 0,
	5, 5, 5, 1,
	16, 2, 0, "RGBA5551", 0 };*/
static constexpr PixelFormatDesc PixelFormatI8 =
{
	0, 0, 0, 0, // shifts
	8, 8, 8, 0, // bits
	8, 1, 0, "I8", PIXEL_I8
};
/*static constexpr PixelFormatDesc PixelFormat5I3A = { 5, 5, 5, 0,
	5, 5, 5, 3,
	8, 1, 0, "IA53", 0 };
static constexpr PixelFormatDesc PixelFormat3A5I = { 0, 0, 0, 5,
	5, 5, 5, 3,
	8, 1, 0, "AI35", 0 };*/
static constexpr PixelFormatDesc PixelFormatIA88 =
{
	8, 8, 8, 0, // shifts
	8, 8, 8, 8, // bits
	16, 2, 0, "IA88", PIXEL_IA88
};

static const PixelFormatDesc *pixelformat_desc(int format)
{
	switch(format)
	{
		case PIXEL_RGB888: return &PixelFormatRGB888;
		case PIXEL_RGBA8888: return &PixelFormatRGBA8888;
		case PIXEL_ARGB8888: return &PixelFormatARGB8888;
		case PIXEL_BGR888: return &PixelFormatBGR888;
		case PIXEL_BGRA8888: return &PixelFormatBGRA8888;
		case PIXEL_ABGR8888: return &PixelFormatABGR8888;
		case PIXEL_RGB565: return &PixelFormatRGB565;
		case PIXEL_ARGB1555: return &PixelFormatARGB1555;
		case PIXEL_I8: return &PixelFormatI8;
		case PIXEL_IA88: return &PixelFormatIA88;
		default : bug_branch("%d", format); return 0;
	}
}

/*
static int pixelformat_fromString(char *str)
{
	for(int i = PIXEL_RGB888; i < PIXEL_END; i++)
	{
		if(string_equal(str, pixelformat_toString(i)))
			return i;
	}
	bugExit("Invalid pixel string in %s", __func__); return(0);
}*/
