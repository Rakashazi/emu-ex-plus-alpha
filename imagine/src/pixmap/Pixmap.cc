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

#define LOGTAG "Pixmap"
#include <imagine/pixmap/Pixmap.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/utility.h>
#include <imagine/util/algorithm.h>
#include <cstring>

namespace IG
{

char *Pixmap::pixel(IG::WP pos) const
{
	return (char*)data + format().offsetBytes(pos.x, pos.y, pitch);
}

void Pixmap::write(const IG::Pixmap &pixmap)
{
	assumeExpr(format() == pixmap.format());
	if(w() == pixmap.w() && !isPadded() && !pixmap.isPadded())
	{
		// whole block
		//logDMsg("copying whole block");
		memcpy(data, pixmap.data, pixmap.pixelBytes());
	}
	else
	{
		// line at a time
		auto srcData = (char*)pixmap.data;
		auto destData = (char*)data;
		uint32_t lineBytes = format().pixelBytes(pixmap.w());
		iterateTimes(pixmap.h(), i)
		{
			memcpy(destData, srcData, lineBytes);
			srcData += pixmap.pitch;
			destData += pitch;
		}
	}
}

void Pixmap::write(const IG::Pixmap &pixmap, IG::WP destPos)
{
	subPixmap(destPos, size() - destPos).write(pixmap);
}

Pixmap Pixmap::subPixmap(IG::WP pos, IG::WP size) const
{
	//logDMsg("sub-pixmap with pos:%dx%d size:%dx%d", pos.x, pos.y, size.x, size.y);
	assumeExpr(pos.x >= 0 && pos.y >= 0);
	assumeExpr(pos.x + size.x <= (int)w() && pos.y + size.y <= (int)h());
	return Pixmap{{size, format()}, pixel(pos), {pitchBytes(), BYTE_UNITS}};
}

Pixmap::operator bool() const
{
	return data;
}

uint32_t Pixmap::pitchPixels() const
{
	return pitch / format().bytesPerPixel();
}

uint32_t Pixmap::pitchBytes() const
{
	return pitch;
}

size_t Pixmap::bytes() const
{
	return pitchBytes() * format().pixelBytes(h());
}

bool Pixmap::isPadded() const
{
	return w() != pitchPixels();
}

uint32_t Pixmap::paddingPixels() const
{
	return pitchPixels() - w();
}

uint32_t Pixmap::paddingBytes() const
{
	return pitchBytes() - format().pixelBytes(w());
}

void Pixmap::clear(IG::WP pos, IG::WP size)
{
	char *destData = pixel(pos);
	if(!isPadded() && (int)w() == size.x)
	{
		std::fill_n(destData, format().pixelBytes(size.x * size.y), 0);
	}
	else
	{
		uint32_t lineBytes = format().pixelBytes(size.x);
		iterateTimes(size.y, i)
		{
			std::fill_n(destData, lineBytes, 0);
			destData += pitch;
		}
	}
}

void Pixmap::clear()
{
	clear({}, {(int)w(), (int)h()});
}

MemPixmap::MemPixmap(PixmapDesc desc):
	Pixmap{desc, nullptr},
	buffer{new uint8_t[desc.format().pixelBytes(desc.w() * desc.h())]}
{
	data = buffer.get();
	//logDMsg("allocated memory pixmap data:%p", data);
}

MemPixmap::MemPixmap(MemPixmap &&o)
{
	*this = std::move(o);
}

MemPixmap &MemPixmap::operator=(MemPixmap &&o)
{
	Pixmap::operator=(o);
	buffer = std::move(o.buffer);
	return *this;
}

}
