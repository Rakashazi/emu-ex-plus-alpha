#pragma once

#include <imagine/config/defs.hh>
#include <imagine/pixmap/PixelDesc.hh>

namespace IG
{

enum PixelFormatID : uint16_t
{
	PIXEL_NONE = 0,
	PIXEL_I8,
	PIXEL_A8,
	PIXEL_IA88,
	PIXEL_RGB565,
	PIXEL_RGBA5551,
	PIXEL_ABGR1555,
	PIXEL_RGBA4444,
	PIXEL_ABGR4444,
	PIXEL_RGB888,
	PIXEL_BGR888,
	PIXEL_RGBA8888,
	PIXEL_BGRA8888,
	PIXEL_ARGB8888,
	PIXEL_ABGR8888,
	PIXEL_RGBX8888,
	PIXEL_END,
	PIXEL_MAX = PIXEL_END - 1
};

static constexpr PixelDesc PIXEL_DESC_NONE			{0, 0, 0, 0, 0,		0, 	0, 	0, 	0, "None"};
static constexpr PixelDesc PIXEL_DESC_I8				{8, 0, 0, 0, 0, 	0, 	0, 	0, 	1, "I8"};
static constexpr PixelDesc PIXEL_DESC_A8  			{0, 0, 0, 8, 0, 	0, 	0, 	0, 	1, "A8"};
static constexpr PixelDesc PIXEL_DESC_IA88  		{8, 0, 0, 8, 8, 	0, 	0, 	0, 	2, "IA88"};
static constexpr PixelDesc PIXEL_DESC_RGB565  	{5, 6, 5, 0, 11,	5, 	0, 	0, 	2, "RGB565"};
static constexpr PixelDesc PIXEL_DESC_RGBA5551 	{5, 5, 5, 1, 11,	6, 	1, 	0, 	2, "RGBA5551"};
static constexpr PixelDesc PIXEL_DESC_ABGR1555 	{5, 5, 5, 1, 0, 	5, 	10, 15,	2, "ABGR1555"};
static constexpr PixelDesc PIXEL_DESC_RGBA4444 	{4, 4, 4, 4, 12,	8, 	4, 	0, 	2, "RGBA4444"};
static constexpr PixelDesc PIXEL_DESC_ABGR4444 	{4, 4, 4, 4, 0, 	4, 	8, 	12, 2, "ABGR4444"};
static constexpr PixelDesc PIXEL_DESC_RGB888 		{8, 8, 8, 0, 16,	8, 	0, 	0, 	3, "RGB888"};
static constexpr PixelDesc PIXEL_DESC_BGR888 		{8, 8, 8, 0, 0, 	8, 	16, 0, 	3, "BGR888"};
static constexpr PixelDesc PIXEL_DESC_RGBA8888 	{8, 8, 8, 8, 24,	16, 8, 	0, 	4, "RGBA8888"};
static constexpr PixelDesc PIXEL_DESC_BGRA8888 	{8, 8, 8, 8, 8, 	16, 24, 0, 	4, "BGRA8888"};
static constexpr PixelDesc PIXEL_DESC_ARGB8888 	{8, 8, 8, 8, 16,	8, 	0, 	24, 4, "ARGB8888"};
static constexpr PixelDesc PIXEL_DESC_ABGR8888 	{8, 8, 8, 8, 0,		8, 	16, 24, 4, "ABGR8888"};
static constexpr PixelDesc PIXEL_DESC_RGBX8888 	{8, 8, 8, 0, 24,	16, 8, 	0, 	4, "RGBX8888"};

class PixelFormat
{
private:
	PixelFormatID id_ = PIXEL_NONE;

public:
	constexpr PixelFormat() {}
	constexpr PixelFormat(PixelFormatID id): id_{id} {}

	constexpr PixelFormatID id() const
	{
		return id_;
	}

	constexpr operator PixelFormatID() const
	{
		return id_;
	}

	constexpr size_t offsetBytes(int x, int y, uint32_t pitchBytes) const
	{
		return desc().offsetBytes(x, y, pitchBytes);
	}

	constexpr size_t pixelBytes(int pixels) const
	{
		return desc().pixelBytes(pixels);
	}

	constexpr uint32_t bytesPerPixel() const
	{
		return desc().bytesPerPixel();
	}

	constexpr uint32_t bitsPerPixel() const
	{
		return desc().bytesPerPixel() * 8;
	}

	constexpr const char *name() const
	{
		return desc().name();
	}

	constexpr bool isGrayscale() const
	{
		return desc().isGrayscale();
	}

	constexpr bool isBGROrder() const
	{
		return desc().isBGROrder();
	}

	constexpr PixelDesc desc() const
	{
		return desc(id_);
	}

	static constexpr PixelDesc desc(PixelFormatID id)
	{
		switch(id)
		{
			case PIXEL_I8: return PIXEL_DESC_I8;
			case PIXEL_A8: return PIXEL_DESC_A8;
			case PIXEL_IA88: return PIXEL_DESC_IA88;
			case PIXEL_RGB565: return PIXEL_DESC_RGB565;
			case PIXEL_RGBA5551: return PIXEL_DESC_RGBA5551;
			case PIXEL_ABGR1555: return PIXEL_DESC_ABGR1555;
			case PIXEL_RGBA4444: return PIXEL_DESC_RGBA4444;
			case PIXEL_ABGR4444: return PIXEL_DESC_ABGR4444;
			case PIXEL_RGB888: return PIXEL_DESC_RGB888;
			case PIXEL_BGR888: return PIXEL_DESC_BGR888;
			case PIXEL_RGBA8888: return PIXEL_DESC_RGBA8888;
			case PIXEL_BGRA8888: return PIXEL_DESC_BGRA8888;
			case PIXEL_ARGB8888: return PIXEL_DESC_ARGB8888;
			case PIXEL_ABGR8888: return PIXEL_DESC_ABGR8888;
			case PIXEL_RGBX8888: return PIXEL_DESC_RGBX8888;
			case PIXEL_NONE: ;
			case PIXEL_END: ;
		}
		return PIXEL_DESC_NONE;
	}
};

static constexpr PixelFormat PIXEL_FMT_NONE{PIXEL_NONE};
static constexpr PixelFormat PIXEL_FMT_I8{PIXEL_I8};
static constexpr PixelFormat PIXEL_FMT_A8{PIXEL_A8};
static constexpr PixelFormat PIXEL_FMT_IA88{PIXEL_IA88};
static constexpr PixelFormat PIXEL_FMT_RGB565{PIXEL_RGB565};
static constexpr PixelFormat PIXEL_FMT_RGBA5551{PIXEL_RGBA5551};
static constexpr PixelFormat PIXEL_FMT_ABGR1555{PIXEL_ABGR1555};
static constexpr PixelFormat PIXEL_FMT_RGBA4444{PIXEL_RGBA4444};
static constexpr PixelFormat PIXEL_FMT_ABGR4444{PIXEL_ABGR4444};
static constexpr PixelFormat PIXEL_FMT_RGB888{PIXEL_RGB888};
static constexpr PixelFormat PIXEL_FMT_BGR888{PIXEL_BGR888};
static constexpr PixelFormat PIXEL_FMT_RGBA8888{PIXEL_RGBA8888};
static constexpr PixelFormat PIXEL_FMT_BGRA8888{PIXEL_BGRA8888};
static constexpr PixelFormat PIXEL_FMT_ARGB8888{PIXEL_ARGB8888};
static constexpr PixelFormat PIXEL_FMT_ABGR8888{PIXEL_ABGR8888};
static constexpr PixelFormat PIXEL_FMT_RGBX8888{PIXEL_RGBX8888};

}
