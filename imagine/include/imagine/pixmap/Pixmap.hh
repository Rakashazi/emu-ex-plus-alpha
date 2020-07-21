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
#include <imagine/util/FunctionTraits.hh>

namespace IG
{

// TODO: rename this class to PixmapView
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
		PixmapDesc{desc},
		pitch{pitch.units == PIXEL_UNITS ? pitch.val * desc.format().bytesPerPixel() : pitch.val},
		data{data}
		{}

	constexpr Pixmap(PixmapDesc desc, void *data):
		Pixmap{desc, data, {desc.w(), PIXEL_UNITS}}
		{}

	char *pixel(WP pos) const;
	void write(Pixmap pixmap);
	void write(Pixmap pixmap, WP destPos);
	void writeConverted(Pixmap pixmap);
	void writeConverted(Pixmap pixmap, WP destPos);
	void clear(WP pos, WP size);
	void clear();
	Pixmap subView(WP pos, WP size) const;
	explicit operator bool() const;
	uint32_t pitchPixels() const;
	uint32_t pitchBytes() const;
	size_t bytes() const;
	bool isPadded() const;
	uint32_t paddingPixels() const;
	uint32_t paddingBytes() const;

	template <class Func>
	static constexpr bool checkTransformFunc()
	{
		constexpr bool isValid = !std::is_void_v<FunctionTraitsR<Func>>
			&& functionTraitsArity<Func> == 1;
		static_assert(isValid, "Transform function must take 1 argument and return a value");
		return isValid;
	}

	template <class Func>
	void writeTransformed(Func func, Pixmap pixmap)
	{
		if constexpr(!checkTransformFunc<Func>())
		{
			return;
		}
		auto srcBytesPerPixel = pixmap.format().bytesPerPixel();
		switch(format().bytesPerPixel())
		{
			bcase 1:
				switch(srcBytesPerPixel)
				{
					case 1: return writeTransformed2<uint8_t,  uint8_t>(func, pixmap);
					case 2: return writeTransformed2<uint16_t, uint8_t>(func, pixmap);
					case 4: return writeTransformed2<uint32_t, uint8_t>(func, pixmap);
				}
			bcase 2:
				switch(srcBytesPerPixel)
				{
					case 1: return writeTransformed2<uint8_t,  uint16_t>(func, pixmap);
					case 2: return writeTransformed2<uint16_t, uint16_t>(func, pixmap);
					case 4: return writeTransformed2<uint32_t, uint16_t>(func, pixmap);
				}
			bcase 4:
				switch(srcBytesPerPixel)
				{
					case 1: return writeTransformed2<uint8_t,  uint32_t>(func, pixmap);
					case 2: return writeTransformed2<uint16_t, uint32_t>(func, pixmap);
					case 4: return writeTransformed2<uint32_t, uint32_t>(func, pixmap);
				}
		}
	}

	template <class Func>
	void writeTransformed(Func func, Pixmap pixmap, WP destPos)
	{
		subView(destPos, size() - destPos).writeTransformed(func, pixmap);
	}

	template <class Src, class Dest, class Func>
	void writeTransformedDirect(Func func, Pixmap pixmap)
	{
		writeTransformed2<Src, Dest>(func, pixmap);
	}

protected:
	uint32_t pitch = 0; // in bytes
	void *data{};

	template <class Src, class Dest, class Func>
	void writeTransformed2(Func func, Pixmap pixmap)
	{
		auto srcData = (Src*)pixmap.data;
		auto destData = (Dest*)data;
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

}
