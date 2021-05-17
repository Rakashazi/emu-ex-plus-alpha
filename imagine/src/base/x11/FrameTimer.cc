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

#define LOGTAG "FrameTimer"
#include <imagine/base/Application.hh>
#include <imagine/logger/logger.h>
#include <memory>

namespace Base
{

FrameTimer XApplication::makeFrameTimer(Screen &screen)
{
	switch(supportedFrameTimer)
	{
		default: return FrameTimer{std::in_place_type<SimpleFrameTimer>, screen};
		case SupportedFrameTimer::DRM: return FrameTimer{std::in_place_type<DRMFrameTimer>, screen};
		case SupportedFrameTimer::FBDEV: return FrameTimer{std::in_place_type<FBDevFrameTimer>, screen};
	}
}

SupportedFrameTimer XApplication::testFrameTimers()
{
	if(DRMFrameTimer::testSupport())
	{
		logMsg("using DRM frame timer");
		return SupportedFrameTimer::DRM;
	}
	if(FBDevFrameTimer::testSupport())
	{
		logMsg("using FBDev frame timer");
		return SupportedFrameTimer::FBDEV;
	}
	logMsg("using simple frame timer");
	return SupportedFrameTimer::SIMPLE;
}

}
