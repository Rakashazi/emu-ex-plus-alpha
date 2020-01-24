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
#include <imagine/pixmap/PixelFormat.hh>
#include <imagine/util/rectangle2.h>
#include <imagine/util/DelegateFunc.hh>
#include <imagine/util/typeTraits.hh>

namespace IG
{

class PixmapDesc : public NotEquals<PixmapDesc>
{
public:
	constexpr PixmapDesc() {}
	constexpr PixmapDesc(WP size, PixelFormat format): w_{(uint32_t)size.x}, h_{(uint32_t)size.y}, format_(format) {}
	constexpr uint32_t w() const { return w_; }
	constexpr uint32_t h() const { return h_; }
	constexpr WP size() const { return {(int)w(), (int)h()}; }
	constexpr PixelFormat format() const { return format_; }
	constexpr size_t pixelBytes() const { return format().pixelBytes(w() * h()); }

	constexpr bool operator ==(const PixmapDesc &rhs) const
	{
		return w_ == rhs.w_ && h_ == rhs.h_ && format_ == rhs.format_;
	}

protected:
	uint32_t w_ = 0, h_ = 0;
	PixelFormat format_{};
};

class Pixmap : public PixmapDesc
{
public:
	enum PitchUnits { PIXEL_UNITS, BYTE_UNITS };
	class PitchInit
	{
	public:
		uint32_t val;
		PitchUnits units;

		constexpr PitchInit(uint32_t pitchVal, PitchUnits units):
			val{pitchVal}, units{units}
			{}
	};

	constexpr Pixmap() {}

	constexpr Pixmap(PixmapDesc desc, void *data, PitchInit pitch):
		PixmapDesc{desc}, data{data},
		pitch{pitch.units == PIXEL_UNITS ? pitch.val * desc.format().bytesPerPixel() : pitch.val}
		{}

	constexpr Pixmap(PixmapDesc desc, void *data):
		Pixmap{desc, data, {desc.w(), PIXEL_UNITS}}
		{}

	char *pixel(IG::WP pos) const;
	void write(const IG::Pixmap &pixmap);
	void write(const IG::Pixmap &pixmap, IG::WP destPos);

	template <class FUNC>
	static constexpr bool checkTransformFunc()
	{
		constexpr bool isValid = std::is_arithmetic<IG::functionTraitsRType<FUNC>>::value
			&& IG::functionTraitsArity<FUNC> == 1;
		static_assert(isValid, "Transform function must take 1 argument and return an arithmetic value");
		return isValid;
	}

	template <class FUNC>
	void writeTransformed(FUNC func, const IG::Pixmap &pixmap)
	{
		if constexpr(!checkTransformFunc<FUNC>())
		{
			return;
		}
		auto srcBytesPerPixel = pixmap.format().bytesPerPixel();
		switch(format().bytesPerPixel())
		{
			bcase 1:
				switch(srcBytesPerPixel)
				{
					bcase 1: writeTransformed2<uint8_t, uint8_t>(func, pixmap);
					bcase 2: writeTransformed2<uint16_t, uint8_t>(func, pixmap);
					bcase 4: writeTransformed2<uint32_t, uint8_t>(func, pixmap);
				}
			bcase 2:
				switch(srcBytesPerPixel)
				{
					bcase 1: writeTransformed2<uint8_t, uint16_t>(func, pixmap);
					bcase 2: writeTransformed2<uint16_t, uint16_t>(func, pixmap);
					bcase 4: writeTransformed2<uint32_t, uint16_t>(func, pixmap);
				}
			bcase 4:
				switch(srcBytesPerPixel)
				{
					bcase 1: writeTransformed2<uint8_t, uint32_t>(func, pixmap);
					bcase 2: writeTransformed2<uint16_t, uint32_t>(func, pixmap);
					bcase 4: writeTransformed2<uint32_t, uint32_t>(func, pixmap);
				}
		}
	}

	template <class FUNC>
	void writeTransformed(FUNC func, const IG::Pixmap &pixmap, IG::WP destPos)
	{
		subPixmap(destPos, size() - destPos).writeTransformed(func, pixmap);
	}

	void clear(IG::WP pos, IG::WP size);
	void clear();
	Pixmap subPixmap(IG::WP pos, IG::WP size) const;
	explicit operator bool() const;
	uint32_t pitchPixels() const;
	uint32_t pitchBytes() const;
	size_t bytes() const;
	bool isPadded() const;
	uint32_t paddingPixels() const;
	uint32_t paddingBytes() const;

protected:
	void *data{};
	uint32_t pitch = 0; // in bytes

	template <class SRC_T, class DEST_T, class FUNC>
	void writeTransformed2(FUNC func, const IG::Pixmap &pixmap)
	{
		auto srcData = (SRC_T*)pixmap.data;
		auto destData = (DEST_T*)data;
		if(w() == pixmap.w() && !isPadded() && !pixmap.isPadded())
		{
			iterateTimes(pixmap.w() * pixmap.h(), i)
			{
				*destData++ = func(*srcData++);
			}
		}
		else
		{
			iterateTimes(pixmap.h(), h)
			{
				auto destLineData = destData;
				auto srcLineData = srcData;
				iterateTimes(pixmap.w(), w)
				{
					*destLineData++ = func(*srcLineData++);
				}
				srcData += pixmap.pitchPixels();
				destData += pitchPixels();
			}
		}
	}
};

class MemPixmap : public Pixmap
{
public:
	MemPixmap() {}
	MemPixmap(PixmapDesc format);
	MemPixmap(MemPixmap &&o);
	MemPixmap &operator=(MemPixmap &&o);
	~MemPixmap();

protected:
	// no copying outside of class
	MemPixmap(const MemPixmap &) = default;
	MemPixmap &operator=(const MemPixmap &) = default;
};

}
