#pragma once

#include <engine-globals.h>
#include <util/pixel.h>

class PixmapDesc
{
public:
	constexpr PixmapDesc(const PixelFormatDesc &format): format(format) { }
	uint x = 0, y = 0;
	uint pitch = 0;
	const PixelFormatDesc &format;

	uint sizeOfNumPixels(uint num) const
	{
		return(num * format.bytesPerPixel);
	}

	uint sizeOfImage() const
	{
		return(x * y * format.bytesPerPixel);
	}

	uint pitchPixels() const
	{
		return pitch / format.bytesPerPixel;
	}

	bool isPadded() const
	{
		return x != pitchPixels();
	}
};

class Pixmap : public PixmapDesc
{
private:
	//uchar byteOrder;
	uchar *nextPixelOnLine(uchar *pixel) const;

public:
	constexpr Pixmap(const PixelFormatDesc &format): PixmapDesc(format) { }
	uchar *data = nullptr;

	uchar *getPixel(uint x, uint y) const;

	void init(uchar *data, uint x, uint y, uint extraPitch = 0)
	{
		this->data = data;
		this->x = x;
		this->y = y;
		this->pitch = (x * format.bytesPerPixel) + extraPitch;
		//logMsg("init %dx%d pixmap, pitch %d", x, y, pitch);
	}

	// version in which Pixmap manages memory, data must be 0 or previously allocated with mem_alloc
	void init(uint x, uint y, uint extraPitch = 0)
	{
		// TODO: realloc
		if(data)
			mem_free(data);
		uchar *data = (uchar*)mem_alloc(x * y * format.bytesPerPixel + extraPitch * y);
		init(data, x, y, extraPitch);
	}

	void deinitManaged()
	{
		mem_free(data);
		data = 0;
	}

	void copy(int srcX, int srcY, int width, int height, Pixmap *dest, int destX, int destY) const;
	void initSubPixmap(const Pixmap &orig, uint x, uint y, uint xlen, uint ylen);
	void copyHLineToRectFromSelf(uint xStart, uint yStart, uint xlen, uint xDest, uint yDest, uint yDestLen);
	void copyVLineToRectFromSelf(uint xStart, uint yStart, uint ylen, uint xDest, uint yDest, uint xDestLen);
	void copyPixelToRectFromSelf(uint xStart, uint yStart, uint xDest, uint yDest, uint xDestLen, uint yDestLen);
	void clearRect(uint xStart, uint yStart, uint xlen, uint ylen);
	void subPixmapOffsets(Pixmap *sub, uint *x, uint *y) const;
};
