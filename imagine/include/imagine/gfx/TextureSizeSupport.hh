#pragma once

#include <imagine/engine-globals.h>
#include <imagine/gfx/defs.hh>
#include <imagine/util/number.h>
#include <algorithm>

class TextureSizeSupport
{
public:
	bool nonPow2 = Config::Gfx::OPENGL_ES_MAJOR_VERSION >= 2;
	static constexpr bool nonSquare = true;
	bool nonPow2CanMipmap = false;
	bool nonPow2CanRepeat = false;
	uint maxXSize = 0, maxYSize = 0;

	constexpr TextureSizeSupport() {}

	bool findBufferXYPixels(uint &x, uint &y, uint imageX, uint imageY)
	{
		using namespace IG;
		if(nonPow2)
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
			x = y = nextHighestPowerOf2(std::max(imageX,imageY));
		}

		if(Config::MACHINE_IS_PANDORA && (x <= 16 || y <= 16))
		{
			// force small textures as square due to PowerVR driver bug
			x = y = std::max(x, y);
		}
		return x == imageX && y == imageY;
	}

	bool supportsMipmaps(uint imageX, uint imageY)
	{
		return imageX && imageY &&
			(nonPow2CanMipmap || (IG::isPowerOf2(imageX) && IG::isPowerOf2(imageY)));
	}
};
