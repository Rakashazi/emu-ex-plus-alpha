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

#include <imagine/gfx/defs.hh>
#include <imagine/util/used.hh>

namespace IG::Gfx
{

class TextureSamplerConfig
{
public:
	bool minLinearFilter = true;
	bool magLinearFilter = true;
	MipFilter mipFilter = MipFilter::LINEAR;
	WrapMode xWrapMode = WrapMode::CLAMP;
	WrapMode yWrapMode = WrapMode::CLAMP;
	ConditionalMember<Config::DEBUG_BUILD, const char *> debugLabel{};

	constexpr void setLinearFilter(bool on) { minLinearFilter = magLinearFilter = on; }
	constexpr void setWrapMode(WrapMode mode) { xWrapMode = yWrapMode = mode; }
	constexpr bool canMipmap() const { return mipFilter != MipFilter::NONE; }
};

	namespace SamplerConfigs
	{

	constexpr TextureSamplerConfig clamp
	{
		.debugLabel = "Clamp"
	};

	constexpr TextureSamplerConfig nearestMipClamp
	{
		.mipFilter = MipFilter::NEAREST,
		.debugLabel = "NearestMipClamp"
	};

	constexpr TextureSamplerConfig noMipClamp
	{
		.mipFilter = MipFilter::NONE,
		.debugLabel = "NoMipClamp"
	};

	constexpr TextureSamplerConfig noLinearNoMipClamp
	{
		.minLinearFilter = false,
		.magLinearFilter = false,
		.mipFilter = MipFilter::NONE,
		.debugLabel = "NoLinearNoMipClamp"
	};

	constexpr TextureSamplerConfig repeat
	{
		.xWrapMode = WrapMode::REPEAT,
		.yWrapMode = WrapMode::REPEAT,
		.debugLabel = "Repeat"
	};

	constexpr TextureSamplerConfig nearestMipRepeat
	{
		.mipFilter = MipFilter::NEAREST,
		.xWrapMode = WrapMode::REPEAT,
		.yWrapMode = WrapMode::REPEAT,
		.debugLabel = "NearestMipRepeat"
	};

	}

}
