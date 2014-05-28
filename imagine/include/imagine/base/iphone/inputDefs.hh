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

#if !defined __ARM_ARCH_6K__
#define CONFIG_INPUT_APPLE_GAME_CONTROLLER
#endif

namespace Input
{

using Time = double; // NSTimeInterval

static Time msToTime(int ms)
{
	return ms / 1000.;
}

using Key = uint8;

namespace Pointer
{
	static const uint LBUTTON = 1;
}

namespace AppleGC
{
	static const uint A = 1,
	B = 2,
	X = 3,
	Y = 4,
	L1 = 5,
	L2 = 6,
	R1 = 7,
	R2 = 8,
	PAUSE = 9,
	UP = 10, RIGHT = 11, DOWN = 12, LEFT = 13,
	LSTICK_UP = 14, LSTICK_RIGHT = 15, LSTICK_DOWN = 16, LSTICK_LEFT = 17,
	RSTICK_UP = 18, RSTICK_RIGHT = 19, RSTICK_DOWN = 20, RSTICK_LEFT = 21
	;

	static const uint COUNT = 22;
}

const char *appleGCButtonName(Key b);

}
