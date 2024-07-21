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
#include <imagine/pixmap/PixmapDesc.hh>
#include <imagine/util/rectangle2.h>
#include <imagine/util/algorithm.h>
#include <imagine/util/ranges.hh>
#include <imagine/util/mdspan.hh>
#include <imagine/util/concepts.hh>
#include <cstring>

namespace IG
{

using RGBTripleArray = std::array<unsigned char, 3>;

RGBTripleArray transformRGB565ToRGB888(uint16_t p);
uint16_t transformRGB888ToRGB565(RGBTripleArray p);
uint32_t transformRGBA8888ToBGRA8888(uint32_t p);
uint16_t transformRGBX8888ToRGB565(uint32_t p);
uint16_t transformBGRX8888ToRGB565(uint32_t p);
RGBTripleArray transformRGBX8888ToRGB888(uint32_t p);
RGBTripleArray transformBGRX8888ToRGB888(uint32_t p);
uint32_t transformRGB565ToRGBX8888(uint16_t p);
uint32_t transformRGB565ToBGRX8888(uint16_t p);
uint32_t transformRGB888ToRGBX8888(RGBTripleArray p);
uint32_t transformRGB888ToBGRX8888(RGBTripleArray p);

template <class Func>
concept PixmapTransformFunc =
		requires (Func &&f, unsigned data){ f(data); } ||
		requires (Func &&f, RGBTripleArray data){ f(data); };

enum class PixmapUnits : uint8_t { PIXEL, BYTE };
struct PitchInit
{
	int val;
	PixmapUnits units;
};

template <class PixData>
class PixmapViewBase
{
public:
	using Units = PixmapUnits;
	static constexpr bool dataIsMutable = !std::is_const_v<PixData>;

	constexpr PixmapViewBase() = default;

	constexpr PixmapViewBase(PixmapDesc desc, Pointer auto data, PitchInit pitch):
		data_{(PixData*)data},
		pitchPx_{pitch.units == Units::PIXEL ? pitch.val : pitch.val / desc.format.bytesPerPixel()},
		desc_{desc} {}

	constexpr PixmapViewBase(PixmapDesc desc, Pointer auto data):
		PixmapViewBase{desc, data, {desc.w(), Units::PIXEL}} {}

	explicit constexpr PixmapViewBase(PixmapDesc desc):
		PixmapViewBase{desc, (PixData*)nullptr} {}

	// Convert non-const PixData version to const
	operator PixmapViewBase<std::add_const_t<PixData>>() const requires(dataIsMutable)
	{
		return {desc(), data(), {pitchPx(), PixmapUnits::PIXEL}};
	}

	constexpr operator bool() const
	{
		return data_;
	}

	constexpr auto data() const
	{
		return data_;
	}

	constexpr PixmapDesc desc() const { return desc_; }
	constexpr int w() const { return desc().w(); }
	constexpr int h() const { return desc().h(); }
	constexpr WSize size() const { return desc().size; }
	constexpr PixelFormat format() const { return desc().format; }

	template<class T>
	constexpr auto mdspan() const
	{
		using Ptr = std::conditional_t<dataIsMutable, T*, const T*>;
		return stridedMdspan2(reinterpret_cast<Ptr>(data()), extents{h(), w()}, pitchPx());
	}

	constexpr PixData *data(WPt pos) const
	{
		switch(format().bytesPerPixel())
		{
			case 1: return (PixData*)&mdspan<uint8_t>()[pos.y, pos.x];
			case 2: return (PixData*)&mdspan<uint16_t>()[pos.y, pos.x];
			case 4: return (PixData*)&mdspan<uint32_t>()[pos.y, pos.x];
		}
		bug_unreachable("invalid bytes per pixel:%d", format().bytesPerPixel());
	}

	constexpr const PixData &operator[](int x, int y) const { return *data({x, y}); }
	constexpr PixData &operator[](int x, int y) requires dataIsMutable { return *data({x, y}); }

	constexpr int pitchPx() const { return pitchPx_; }
	constexpr int pitchBytes() const { return pitchPx() * format().bytesPerPixel(); }

	constexpr int bytes() const
	{
		return pitchBytes() * h();
	}

	constexpr int unpaddedBytes() const
	{
		return desc().bytes();
	}

	constexpr bool isPadded() const
	{
		return w() != pitchPx();
	}

	constexpr int paddingPixels() const
	{
		return pitchPx() - w();
	}

	constexpr int paddingBytes() const
	{
		return pitchBytes() - format().pixelBytes(w());
	}

	constexpr PixmapViewBase subView(WPt pos, WSize size) const
	{
		//logDMsg("sub-pixmap with pos:%dx%d size:%dx%d", pos.x, pos.y, size.x, size.y);
		assumeExpr(pos.x >= 0 && pos.y >= 0);
		assumeExpr(pos.x + size.x <= w() && pos.y + size.y <= h());
		return PixmapViewBase{desc().makeNewSize(size), data(pos), {pitchBytes(), Units::BYTE}};
	}

	void write(auto pixmap) requires(dataIsMutable)
	{
		assumeExpr(format() == pixmap.format());
		if(w() == pixmap.w() && !isPadded() && !pixmap.isPadded())
		{
			// whole block
			//logDMsg("copying whole block");
			memcpy(data_, pixmap.data(), pixmap.unpaddedBytes());
		}
		else
		{
			// line at a time
			auto srcData = pixmap.data();
			auto destData = data();
			auto destPitch = pitchBytes();
			auto lineBytes = format().pixelBytes(pixmap.w());
			for([[maybe_unused]] auto i : iotaCount(pixmap.h()))
			{
				memcpy(destData, srcData, lineBytes);
				srcData += pixmap.pitchBytes();
				destData += destPitch;
			}
		}
	}

	void write(auto pixmap, WPt destPos) requires(dataIsMutable)
	{
		subView(destPos, size() - destPos).write(pixmap);
	}

	void writeConverted(auto pixmap) requires(dataIsMutable)
	{
		if(format() == pixmap.format())
		{
			write(pixmap);
			return;
		}
		auto srcFormatID = pixmap.format().id;
		switch(format().id)
		{
			case PixelFormatId::RGBA8888:
				switch(srcFormatID)
				{
					case PixelFormatId::BGRA8888: return convertRGBA8888ToBGRA8888(*this, pixmap);
					case PixelFormatId::RGB565: return convertRGB565ToRGBX8888(*this, pixmap);
					case PixelFormatId::RGB888: return convertRGB888ToRGBX8888(*this, pixmap);
					default: return invalidFormatConversion(*this, pixmap);
				}
			case PixelFormatId::BGRA8888:
				switch(srcFormatID)
				{
					case PixelFormatId::RGBA8888: return convertRGBA8888ToBGRA8888(*this, pixmap);
					case PixelFormatId::RGB565: return convertRGB565ToBGRX8888(*this, pixmap);
					case PixelFormatId::RGB888: return convertRGB888ToBGRX8888(*this, pixmap);
					default: return invalidFormatConversion(*this, pixmap);
				}
			case PixelFormatId::RGB888:
				switch(srcFormatID)
				{
					case PixelFormatId::BGRA8888: return convertBGRX8888ToRGB888(*this, pixmap);
					case PixelFormatId::RGBA8888: return convertRGBX8888ToRGB888(*this, pixmap);
					case PixelFormatId::RGB565: return convertRGB565ToRGB888(*this, pixmap);
					default: return invalidFormatConversion(*this, pixmap);
				}
			case PixelFormatId::RGB565:
				switch(srcFormatID)
				{
					case PixelFormatId::RGBA8888: return convertRGBX8888ToRGB565(*this, pixmap);
					case PixelFormatId::BGRA8888: return convertBGRX8888ToRGB565(*this, pixmap);
					case PixelFormatId::RGB888: return convertRGB888ToRGB565(*this, pixmap);
					default: return invalidFormatConversion(*this, pixmap);
				}
			default:
				return invalidFormatConversion(*this, pixmap);
		}
	}

	void writeConverted(auto pixmap, WPt destPos) requires(dataIsMutable)
	{
		subView(destPos, size() - destPos).writeConverted(pixmap);
	}

	void clear(WPt pos, WSize size) requires(dataIsMutable)
	{
		char *destData = data(pos);
		if(!isPadded() && (int)w() == size.x)
		{
			std::fill_n(destData, format().pixelBytes(size.x * size.y), 0);
		}
		else
		{
			auto lineBytes = format().pixelBytes(size.x);
			auto pitch = pitchBytes();
			for([[maybe_unused]] auto i : iotaCount(size.y))
			{
				std::fill_n(destData, lineBytes, 0);
				destData += pitch;
			}
		}
	}

	void clear() requires(dataIsMutable)
	{
		clear({}, {(int)w(), (int)h()});
	}

	void transformInPlace(PixmapTransformFunc auto &&func) requires(dataIsMutable)
	{
		switch(format().bytesPerPixel())
		{
			case 1: return transformInPlace2<uint8_t>(func);
			case 2: return transformInPlace2<uint16_t>(func);
			case 4: return transformInPlace2<uint32_t>(func);
		}
	}

	template <class Data>
	void transformInPlace2(PixmapTransformFunc auto &&func) requires(dataIsMutable)
	{
		auto data = (Data*)data_;
		if(!isPadded())
		{
			transformNOverlapped(data, w() * h(), data,
				[=](Data pixel)
				{
					return func(pixel);
				});
		}
		else
		{
			auto dataPitchPixels = pitchPx();
			auto width = w();
			for([[maybe_unused]] auto y : iotaCount(h()))
			{
				auto lineData = data;
				transformNOverlapped(lineData, width, lineData,
					[=](Data pixel)
					{
						return func(pixel);
					});
				data += dataPitchPixels;
			}
		}
	}

	void writeTransformed(PixmapTransformFunc auto &&func, auto pixmap) requires(dataIsMutable)
	{
		auto srcBytesPerPixel = pixmap.format().bytesPerPixel();
		switch(format().bytesPerPixel())
		{
			case 1: return writeTransformed<uint8_t>(srcBytesPerPixel, IG_forward(func), pixmap);
			case 2: return writeTransformed<uint16_t>(srcBytesPerPixel, IG_forward(func), pixmap);
			case 4: return writeTransformed<uint32_t>(srcBytesPerPixel, IG_forward(func), pixmap);
		}
	}

	void writeTransformed(PixmapTransformFunc auto &&func, auto pixmap, WPt destPos) requires(dataIsMutable)
	{
		subView(destPos, size() - destPos).writeTransformed(func, pixmap);
	}

	template <class Src, class Dest>
	void writeTransformedDirect(PixmapTransformFunc auto &&func, auto pixmap) requires(dataIsMutable)
	{
		writeTransformed2<Src, Dest>(func, pixmap);
	}

protected:
	PixData *data_{};
	int pitchPx_{};
	PixmapDesc desc_{};

	template <class Dest>
	void writeTransformed(uint8_t srcBytesPerPixel, PixmapTransformFunc auto &&func, auto pixmap) requires(dataIsMutable)
	{
		switch(srcBytesPerPixel)
		{
			case 1: return writeTransformed2<uint8_t,  Dest>(IG_forward(func), pixmap);
			case 2: return writeTransformed2<uint16_t, Dest>(IG_forward(func), pixmap);
			case 4: return writeTransformed2<uint32_t, Dest>(IG_forward(func), pixmap);
		}
	};

	template <class Src, class Dest>
	void writeTransformed2(PixmapTransformFunc auto &&func, auto pixmap) requires(dataIsMutable)
	{
		auto srcData = (const Src*)pixmap.data();
		auto destData = (Dest*)data_;
		if(w() == pixmap.w() && !isPadded() && !pixmap.isPadded())
		{
			transformN(srcData, pixmap.w() * pixmap.h(), destData,
				[=](Src srcPixel)
				{
					return func(srcPixel);
				});
		}
		else
		{
			auto srcPitchPixels = pixmap.pitchPx();
			auto destPitchPixels = pitchPx();
			for([[maybe_unused]] auto h : iotaCount(pixmap.h()))
			{
				auto destLineData = destData;
				auto srcLineData = srcData;
				transformN(srcLineData, pixmap.w(), destLineData,
					[=](Src srcPixel)
					{
						return func(srcPixel);
					});
				srcData += srcPitchPixels;
				destData += destPitchPixels;
			}
		}
	}

	static void invalidFormatConversion([[maybe_unused]] auto dest, [[maybe_unused]] auto src)
	{
		bug_unreachable("unimplemented conversion:%s -> %s", src.format().name(), dest.format().name());
	}

	static void convertRGB888ToRGBX8888(auto dest, auto src)
	{
		dest.template writeTransformedDirect<RGBTripleArray, uint32_t>(transformRGB888ToRGBX8888, src);
	}

	static void convertRGB888ToBGRX8888(auto dest, auto src)
	{
		dest.template writeTransformedDirect<RGBTripleArray, uint32_t>(transformRGB888ToBGRX8888, src);
	}

	static void convertRGB565ToRGBX8888(auto dest, auto src)
	{
		dest.writeTransformed(transformRGB565ToRGBX8888, src);
	}

	static void convertRGB565ToBGRX8888(auto dest, auto src)
	{
		dest.writeTransformed(transformRGB565ToBGRX8888, src);
	}

	static void convertRGBX8888ToRGB888(auto dest, auto src)
	{
		dest.template writeTransformedDirect<uint32_t, RGBTripleArray>(transformRGBX8888ToRGB888, src);
	}

	static void convertBGRX8888ToRGB888(auto dest, auto src)
	{
		dest.template writeTransformedDirect<uint32_t, RGBTripleArray>(transformBGRX8888ToRGB888, src);
	}

	static void convertRGB565ToRGB888(auto dest, auto src)
	{
		dest.template writeTransformedDirect<uint16_t, RGBTripleArray>(transformRGB565ToRGB888, src);
	}

	static void convertRGB888ToRGB565(auto dest, auto src)
	{
		dest.template writeTransformedDirect<RGBTripleArray, uint16_t>(transformRGB888ToRGB565, src);
	}

	static void convertRGBX8888ToRGB565(auto dest, auto src)
	{
		dest.writeTransformed(transformRGBX8888ToRGB565, src);
	}

	static void convertRGBA8888ToBGRA8888(auto dest, auto src)
	{
		dest.writeTransformed(transformRGBA8888ToBGRA8888, src);
	}

	static void convertBGRX8888ToRGB565(auto dest, auto src)
	{
		dest.template writeTransformed(transformBGRX8888ToRGB565, src);
	}
};

using PixmapView = PixmapViewBase<const char>;
using MutablePixmapView = PixmapViewBase<char>;

}
