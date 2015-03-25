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

#include <imagine/engine-globals.h>
#include <imagine/pixmap/PixelFormat.hh>
#include <imagine/util/rectangle2.h>
#include <imagine/util/DelegateFunc.hh>

namespace IG
{

class PixmapDesc : public NotEquals<PixmapDesc>
{
public:
	constexpr PixmapDesc() {}
	constexpr PixmapDesc(WP size, PixelFormat format): w_{(uint)size.x}, h_{(uint)size.y}, format_(format) {}
	constexpr uint w() const { return w_; }
	constexpr uint h() const { return h_; }
	constexpr WP size() const { return {(int)w(), (int)h()}; }
	constexpr PixelFormat format() const { return format_; }
	constexpr uint pixelBytes() const { return format().pixelBytes(w() * h()); }

	constexpr bool operator ==(const PixmapDesc &rhs) const
	{
		return w_ == rhs.w_ && h_ == rhs.h_ && format_ == rhs.format_;
	}

protected:
	uint w_ = 0, h_ = 0;
	PixelFormat format_;
};

class Pixmap : public PixmapDesc
{
public:
	enum PitchUnits { PIXEL_UNITS, BYTE_UNITS };
	class PitchInit
	{
	public:
		uint val;
		PitchUnits units;

		constexpr PitchInit(uint pitchVal, PitchUnits units):
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
	void write(const IG::Pixmap &pixmap, IG::WP destPos);
	void clear(IG::WP pos, IG::WP size);
	void clear();
	Pixmap subPixmap(IG::WP pos, IG::WP size) const;
	explicit operator bool() const;
	uint pitchPixels() const;
	uint pitchBytes() const;
	uint bytes() const;
	bool isPadded() const;

protected:
	void *data{};
	uint pitch = 0; // in bytes
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
