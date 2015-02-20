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
#include <imagine/util/pixel.h>
#include <imagine/util/fixed.hh>
#include <cstring>

namespace IG
{

char *Pixmap::getPixel(uint x, uint y) const
{
	return(data + format.offsetBytes(x, y, pitch));
}

void Pixmap::copy(int srcX, int srcY, int width, int height, Pixmap &dest, int destX, int destY) const
{
	//logDMsg("copying pixmap format src: %s dest: %s", pixelformat_toString(src->format), pixelformat_toString(dest->format));
	// copy all width/height if 0
	if(!width) width = x;
	if(!height) height = y;
	//logDMsg("from %dx%d img to dest %dx%d img src: x %d-%d y %d-%d to dest: %dx%d", x, y, dest.x, dest.y, srcX, width, srcY, height, destX, destY);
	assert(width <= (int)dest.x + destX);
	assert(height <= (int)dest.y + destY);
	if(data == dest.data)
	{
		assert(dest.format.bytesPerPixel <= format.bytesPerPixel);
	}
	//logMsg("copying %s to %s", dest->format->name, format->name);
	assert(format.id == dest.format.id);
	char *srcData = getPixel(srcX, srcY);
	char *destData = dest.getPixel(destX, destY);
	if(dest.x == x && dest.pitch == pitch && dest.x == (uint)width)
	{
		// whole block
		//logDMsg("copying whole block");
		memcpy(destData, srcData, sizeOfPixels(width * height));
	}
	else
	{
		// line at a time
		for(int y = srcY; y < height; y++)
		{
			memcpy(destData, srcData, sizeOfPixels(width));
			srcData += pitch;
			destData += dest.pitch;
		}
	}
}

void Pixmap::initSubPixmap(const Pixmap &orig, uint x, uint y, uint xlen, uint ylen)
{
	data = orig.getPixel(x, y);
	this->x = xlen;
	this->y = ylen;
	pitch = orig.pitch;
}

Pixmap Pixmap::makeSubPixmap(IG::WP offset, IG::WP size)
{
	assert(offset.x >= 0 && offset.y >= 0);
	assert(offset.x + size.x <= (int)x && offset.y + size.y <= (int)y);
	Pixmap sub{*this};
	sub.initSubPixmap(*this, offset.x, offset.y, size.x, size.y);
	return sub;
}

void Pixmap::clearRect(uint xStart, uint yStart, uint xlen, uint ylen)
{
	char *destPixel = Pixmap::getPixel(xStart, yStart+y);
	uint clearBytes = format.bytesPerPixel * xlen;
	iterateTimes(ylen, i)
	{
		memset(destPixel, 0, clearBytes);
		destPixel += pitch;
	}
}

}
