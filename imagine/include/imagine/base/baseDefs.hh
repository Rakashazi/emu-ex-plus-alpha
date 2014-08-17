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

#include <imagine/engine-globals.h>
#include <imagine/util/DelegateFunc.hh>
#include <imagine/util/bits.h>

namespace Base
{
using namespace IG;

#if defined __APPLE__ && TARGET_OS_IPHONE
using FrameTimeBase = double;

constexpr static double decimalFrameTimeBaseFromSec(double sec)
{
	return sec;
}

constexpr static FrameTimeBase frameTimeBaseFromSec(double sec)
{
	return sec;
}
#else
using FrameTimeBase = uint64_t;

constexpr static double decimalFrameTimeBaseFromSec(double sec)
{
	return sec * (double)1000000000.;
}

constexpr static FrameTimeBase frameTimeBaseFromSec(double sec)
{
	return decimalFrameTimeBaseFromSec(sec);
}
#endif

// orientation
static constexpr uint VIEW_ROTATE_0 = bit(0), VIEW_ROTATE_90 = bit(1), VIEW_ROTATE_180 = bit(2), VIEW_ROTATE_270 = bit(3);
static constexpr uint VIEW_ROTATE_AUTO = bit(5);
static constexpr uint VIEW_ROTATE_ALL = VIEW_ROTATE_0 | VIEW_ROTATE_90 | VIEW_ROTATE_180 | VIEW_ROTATE_270;
static constexpr uint VIEW_ROTATE_ALL_BUT_UPSIDE_DOWN = VIEW_ROTATE_0 | VIEW_ROTATE_90 | VIEW_ROTATE_270;

const char *orientationToStr(uint o);
bool orientationIsSideways(uint o);
uint validateOrientationMask(uint oMask);
}
