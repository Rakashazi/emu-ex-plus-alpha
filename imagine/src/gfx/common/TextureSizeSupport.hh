#pragma once

#include <util/number.h>

class TextureSizeSupport
{
public:
	bool nonPow2;
	bool nonSquare;
	bool filtering;
	bool nonPow2CanMipmap;
	uint minXSize, minYSize;
	uint maxXSize, maxYSize;

	static const uint streamHint = BIT(0);

	void findBufferXYPixels(uint &x, uint &y, uint imageX, uint imageY, uint hints = 0)
	{
		using namespace IG;
		if(nonPow2
			/*#ifndef CONFIG_BASE_PS3
				&& (hints & streamHint) // don't use npot for static use textures
			#endif*/
			)
		{
			x = imageX;
			y = imageY;
		}
		else if(nonSquare)
		{
			x = nextHighestPowerOf2(imageX);
			y = nextHighestPowerOf2(imageY);
		}
		else
		{
			x = nextHighestPowerOf2(IG::max(imageX,imageY));
			y = nextHighestPowerOf2(IG::max(imageX,imageY));
		}

		if(minXSize && x < minXSize) x = minXSize;
		if(minYSize && y < minYSize) y = minYSize;
	}

	bool supportsMipmaps(uint imageX, uint imageY)
	{
		// TODO: setup and use nonPow2CanMipmap variable
		return IG::isPowerOf2(imageX) && IG::isPowerOf2(imageY);
	}
};
