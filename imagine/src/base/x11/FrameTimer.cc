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

#include <imagine/base/Application.hh>
#include <imagine/logger/logger.h>
#include <memory>

namespace IG
{

constexpr SystemLogger log{"FrameTimer"};

void XApplication::emplaceFrameTimer(FrameTimer &t, Screen &screen, bool useVariableTime)
{
	if(useVariableTime)
	{
		t.emplace<SimpleFrameTimer>(screen);
	}
	else
	{
		switch(supportedFrameTimer)
		{
			default: t.emplace<SimpleFrameTimer>(screen); break;
			#if CONFIG_PACKAGE_LIBDRM
			case SupportedFrameTimer::DRM: t.emplace<DRMFrameTimer>(screen); break;
			#endif
			case SupportedFrameTimer::FBDEV: t.emplace<FBDevFrameTimer>(screen); break;
		}
	}
}

SupportedFrameTimer XApplication::testFrameTimers()
{
	#if CONFIG_PACKAGE_LIBDRM
	if(DRMFrameTimer::testSupport())
	{
		log.info("using DRM frame timer");
		return SupportedFrameTimer::DRM;
	}
	#endif
	if(FBDevFrameTimer::testSupport())
	{
		log.info("using FBDev frame timer");
		return SupportedFrameTimer::FBDEV;
	}
	log.info("using simple frame timer");
	return SupportedFrameTimer::SIMPLE;
}

}
