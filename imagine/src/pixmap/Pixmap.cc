#define thisModuleName "pixmap"
#include <pixmap/Pixmap.hh>
#include <util/pixel.h>
#include <util/fixed.hh>
#include <string.h>

uchar *Pixmap::getPixel(uint x, uint y) const
{
	return(data + format.offsetBytes(x, y, pitch));
}

uchar *Pixmap::nextPixelOnLine(uchar *pixel) const
{
	return(pixel + format.bytesPerPixel);
}

void Pixmap::subPixmapOffsets(Pixmap *sub, uint *x, uint *y) const
{
	ptrsize dataOffset = sub->data - data;
	assert(dataOffset < Pixmap::sizeOfImage());
	*x = ((uint)dataOffset % pitch) / format.bytesPerPixel;
	*y = (uint)dataOffset / pitch;
}

void Pixmap::copy(int srcX, int srcY, int width, int height, Pixmap *dest, int destX, int destY) const
{
	//logDMsg("copying pixmap format src: %s dest: %s", pixelformat_toString(src->format), pixelformat_toString(dest->format));
	// copy all width/height if 0
	if(width == 0) width = x;
	if(height == 0) height = y;
	//logDMsg("from %dx%d img src: x %d-%d y %d-%d to dest: %dx%d", x, y, srcX, width, srcY, height, destX, destY);
	assert(width <= (int)dest->x + destX);
	assert(height <= (int)dest->y + destY);
	if(data == dest->data)
	{
		assert(dest->format.bytesPerPixel <= format.bytesPerPixel);
	}
	//logMsg("copying %s to %s", dest->format->name, format->name);
	assert(format.id == dest->format.id);
	//if(format->id == dest->format->id)
	{
		uchar *srcData = getPixel(srcX, srcY);
		uchar *destData = dest->getPixel(destX, destY);
		if(dest->x == x && pitch == dest->pitch && dest->x == (uint)width)
		{
			// whole block
			//logDMsg("copying whole block");
			memcpy(destData, srcData, format.bytesPerPixel * width * height);
		}
		else
		{
			// line at a time
			for(int y = srcY; y < height; y++)
			{
				//memcpy(dest->getPixel(destX, destY+y), getPixel(srcX, y), format->bytesPerPixel * width);
				memcpy(destData, srcData, format.bytesPerPixel * width);
				srcData += pitch;
				destData += dest->pitch;
			}
		}
	}
	/*else
	{

	}*/
}

void Pixmap::initSubPixmap(const Pixmap &orig, uint x, uint y, uint xlen, uint ylen)
{
	this->data = orig.getPixel(x, y);
	this->x = xlen;
	this->y = ylen;
	this->pitch = orig.pitch;
}

void Pixmap::copyHLineToRectFromSelf(uint xStart, uint yStart, uint xlen, uint xDest, uint yDest, uint yDestLen)
{
	uchar *srcLine = Pixmap::getPixel(xStart, yStart);
	uint runLen = xlen * format.bytesPerPixel;
	for(uint y = 0; y < yDestLen; y++)
	{
		uchar *destline = Pixmap::getPixel(xDest, yDest+y);
		memcpy(destline, srcLine, runLen);
	}
}

void Pixmap::copyVLineToRectFromSelf(uint xStart, uint yStart, uint ylen, uint xDest, uint yDest, uint xDestLen)
{
	for(uint y = 0; y < ylen; y++)
	{
		uchar *srcPixel = Pixmap::getPixel(xStart, yStart+y);
		uchar *destPixel = Pixmap::getPixel(xDest, yDest+y);
		for(uint x = 0; x < xDestLen; x++)
		{
			memcpy(destPixel, srcPixel, format.bytesPerPixel);
			destPixel = Pixmap::nextPixelOnLine(destPixel);
		}
	}
}

void Pixmap::copyPixelToRectFromSelf(uint xStart, uint yStart, uint xDest, uint yDest, uint xDestLen, uint yDestLen)
{
	uchar *srcPixel = Pixmap::getPixel(xStart, yStart);
	for(uint y = 0; y < yDestLen; y++)
	{
		uchar *destPixel = Pixmap::getPixel(xDest, yDest+y);
		for(uint x = 0; x < xDestLen; x++)
		{
			memcpy(destPixel, srcPixel, format.bytesPerPixel);
			destPixel = Pixmap::nextPixelOnLine(destPixel);
		}
	}
}

void Pixmap::clearRect(uint xStart, uint yStart, uint xlen, uint ylen)
{
	for(uint y = 0; y < ylen; y++)
	{
		uchar *destPixel = Pixmap::getPixel(xStart, yStart+y);
		memset(destPixel, 0, format.bytesPerPixel * xlen);
	}
}
