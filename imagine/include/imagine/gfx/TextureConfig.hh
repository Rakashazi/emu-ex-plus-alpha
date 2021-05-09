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

class TextureSampler;

class TextureConfig
{
public:
	constexpr TextureConfig() {}
	constexpr TextureConfig(IG::PixmapDesc pixDesc, const TextureSampler *compatSampler = {}):
		compatSampler_{compatSampler}, pixmapDesc_{pixDesc}
	{}

	constexpr void setLevels(uint8_t levels)
	{
		levels_ = levels;
	}

	constexpr void setAllLevels()
	{
		levels_ = 0;
	}

	constexpr uint8_t levels() const
	{
		return levels_;
	}

	constexpr void setWillGenerateMipmaps(bool on)
	{
		genMipmaps = on;
		if(on)
			setAllLevels();
	}

	constexpr bool willGenerateMipmaps() const
	{
		return genMipmaps;
	}

	constexpr void setPixmapDesc(IG::PixmapDesc pixDesc)
	{
		pixmapDesc_ = pixDesc;
	}

	constexpr IG::PixmapDesc pixmapDesc() const
	{
		return pixmapDesc_;
	}

	constexpr void setCompatSampler(const TextureSampler *sampler)
	{
		compatSampler_ = sampler;
	}

	constexpr const TextureSampler *compatSampler() const
	{
		return compatSampler_;
	}

	constexpr void setColorSpace(ColorSpace c) { colorSpace_ = c; }

	constexpr ColorSpace colorSpace() const { return colorSpace_; }

private:
	const TextureSampler *compatSampler_{};
	IG::PixmapDesc pixmapDesc_{};
	uint8_t levels_{1};
	bool genMipmaps{};
	ColorSpace colorSpace_{};
};

}
