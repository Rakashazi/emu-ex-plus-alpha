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
#include <imagine/gfx/defs.hh>
#include <imagine/pixmap/PixmapDesc.hh>

namespace Gfx
{

class TextureConfig
{
public:
	constexpr TextureConfig() {}
	constexpr TextureConfig(IG::PixmapDesc pixDesc): pixmapDesc_{pixDesc} {}

	void setLevels(uint32_t levels)
	{
		levels_ = levels;
	}

	void setAllLevels()
	{
		levels_ = 0;
	}

	uint32_t levels()
	{
		return levels_;
	}

	void setWillGenerateMipmaps(bool on)
	{
		genMipmaps = on;
		if(on)
			setAllLevels();
	}

	bool willGenerateMipmaps() const
	{
		return genMipmaps;
	}

	void setPixmapDesc(IG::PixmapDesc pixDesc)
	{
		pixmapDesc_ = pixDesc;
	}

	IG::PixmapDesc pixmapDesc() const
	{
		return pixmapDesc_;
	}

private:
	uint32_t levels_ = 1;
	IG::PixmapDesc pixmapDesc_;
	bool genMipmaps = false;
};

}
