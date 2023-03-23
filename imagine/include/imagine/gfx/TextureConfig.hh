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
#include <imagine/gfx/TextureSamplerConfig.hh>
#include <imagine/pixmap/PixmapDesc.hh>

namespace IG::Gfx
{

class TextureConfig
{
public:
	TextureSamplerConfig samplerConfig{};
	PixmapDesc pixmapDesc{};
	uint8_t levels{1};
	bool genMipmaps{};
	ColorSpace colorSpace{};

	constexpr TextureConfig() = default;
	constexpr TextureConfig(PixmapDesc pixDesc, TextureSamplerConfig samplerConfig = {}):
		samplerConfig{samplerConfig}, pixmapDesc{pixDesc} {}
	constexpr void setAllLevels() { levels = 0; }
	constexpr bool willGenerateMipmaps() const { return genMipmaps; }

	constexpr void setWillGenerateMipmaps(bool on)
	{
		genMipmaps = on;
		if(on)
			setAllLevels();
	}
};

}
