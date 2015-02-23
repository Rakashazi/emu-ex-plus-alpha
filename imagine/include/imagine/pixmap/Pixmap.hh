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
#include <imagine/util/pixel.h>
#include <imagine/util/rectangle2.h>

namespace IG
{

class PixmapDesc
{
public:
	uint x = 0, y = 0;
	uint pitch = 0; // bytes per line
	PixelFormatDesc format;

	constexpr PixmapDesc(PixelFormatDesc format): format(format) {}

	uint sizeOfPixels(uint pixels) const
	{
		return pixels * format.bytesPerPixel;
	}

	uint size() const
	{
		return pitch * (y * format.bytesPerPixel);
	}

	uint unpaddedSize() const
	{
		return x * y * format.bytesPerPixel;
	}

	uint pitchPixels() const
	{
		return pitch / format.bytesPerPixel;
	}

	bool isPadded() const
	{
		return x != pitchPixels();
	}

	bool isSameGeometry(PixmapDesc other)
	{
		return x == other.x && y == other.y;
	}
};

class Pixmap : public PixmapDesc
{
public:
	char *data{};

	constexpr Pixmap(PixelFormatDesc format): PixmapDesc{format} {}
	constexpr Pixmap(PixmapDesc desc): PixmapDesc{desc} {}
	char *getPixel(uint x, uint y) const;

	void init2(void *data, uint x, uint y, uint pitch)
	{
		this->data = (char*)data;
		this->x = x;
		this->y = y;
		this->pitch = pitch;
		//logMsg("init %dx%d pixmap, pitch %d", x, y, pitch);
	}

	void init(void *data, uint x, uint y)
	{
		init2(data, x, y, x * format.bytesPerPixel);
	}

	void copy(int srcX, int srcY, int width, int height, Pixmap &dest, int destX, int destY) const;
	void clearRect(uint xStart, uint yStart, uint xlen, uint ylen);
	void initSubPixmap(const Pixmap &orig, uint x, uint y, uint xlen, uint ylen);
	Pixmap makeSubPixmap(IG::WP offset, IG::WP size);
	uint size() const { return y * pitch; }
	explicit operator bool() const { return data; }
};

class StaticManagedPixmap : public Pixmap
{
public:
	constexpr StaticManagedPixmap(PixelFormatDesc format): Pixmap{format} {}

	bool init(uint x, uint y)
	{
		deinit();
		data = (char*)mem_alloc(x * y * format.bytesPerPixel);
		if(!data)
			return false;
		Pixmap::init(data, x, y);
		return true;
	}

	void deinit()
	{
		if(data)
		{
			mem_free(data);
			data = nullptr;
		}
	}
};

class ManagedPixmap : public StaticManagedPixmap
{
public:
	constexpr ManagedPixmap(PixelFormatDesc format): StaticManagedPixmap{format} {}
	~ManagedPixmap()
	{
		deinit();
	}
};

}
