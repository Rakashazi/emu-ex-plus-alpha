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

namespace Gfx
{

class TextureSamplerConfig
{
private:
	bool minLinearFiltering = true;
	bool magLinearFiltering = true;
	MipFilterMode mipFiltering = MIP_FILTER_LINEAR;
	WrapMode xWrapMode_ = WRAP_CLAMP;
	WrapMode yWrapMode_ = WRAP_CLAMP;
	#ifndef NDEBUG
	const char *label{};
	#endif

public:
	constexpr TextureSamplerConfig() {}

	void setLinearFilter(bool on)
	{
		minLinearFiltering = magLinearFiltering = on;
	}

	void setMinLinearFilter(bool on)
	{
		minLinearFiltering = on;
	}

	void setMagLinearFilter(bool on)
	{
		magLinearFiltering = on;
	}

	bool minLinearFilter() const
	{
		return minLinearFiltering;
	}

	bool magLinearFilter() const
	{
		return magLinearFiltering;
	}

	void setMipFilter(MipFilterMode filter)
	{
		mipFiltering = filter;
	}

	MipFilterMode mipFilter() const
	{
		return mipFiltering;
	}

	void setWrapMode(WrapMode mode)
	{
		xWrapMode_ = yWrapMode_ = mode;
	}

	void setXWrapMode(WrapMode mode)
	{
		xWrapMode_ = mode;
	}

	void setYWrapMode(WrapMode mode)
	{
		yWrapMode_ = mode;
	}

	WrapMode xWrapMode() const
	{
		return xWrapMode_;
	}

	WrapMode yWrapMode() const
	{
		return yWrapMode_;
	}

	void setDebugLabel(const char *str)
	{
		#ifndef NDEBUG
		label = str;
		#endif
	}

	const char *debugLabel()
	{
		#ifndef NDEBUG
		return label;
		#else
		return nullptr;
		#endif
	}

	static TextureSamplerConfig makeWithVideoUseConfig()
	{
		TextureSamplerConfig config;
		config.setLinearFilter(true);
		config.setMipFilter(MIP_FILTER_NONE);
		config.setWrapMode(WRAP_CLAMP);
		return config;
	}
};

}
