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
#include <imagine/input/config.hh>

namespace Input
{

static constexpr uint8 MAX_BLUETOOTH_DEVS_PER_TYPE = 5;

	namespace PS3
	{
	static const uint CROSS = 1,
	CIRCLE = 2,
	SQUARE = 3,
	TRIANGLE = 4,
	L1 = 5,
	L2 = 6,
	L3 = 7,
	R1 = 8,
	R2 = 9,
	R3 = 10,
	SELECT = 11,
	START = 12,
	UP = 13, RIGHT = 14, DOWN = 15, LEFT = 16,
	PS = 17,
	LSTICK_RIGHT = 18, LSTICK_LEFT = 19, LSTICK_DOWN = 20, LSTICK_UP = 21,
	RSTICK_RIGHT = 22, RSTICK_LEFT = 23, RSTICK_DOWN = 24, RSTICK_UP = 25
	;

	static const uint COUNT = 26;
	}

	namespace Wiimote
	{
	static const uint PLUS = 1,
	MINUS = 2,
	HOME = 3,
	LEFT = 4, RIGHT = 5, UP = 6, DOWN = 7,
	_1 = 8,
	_2 = 9,
	A = 10,
	B = 11,
	// Nunchuk
	NUN_C = 12, NUN_Z = 13,
	NUN_STICK_LEFT = 14, NUN_STICK_RIGHT = 15, NUN_STICK_UP = 16, NUN_STICK_DOWN = 17
	;

	static const uint COUNT = 18;
	}

	namespace WiiCC
	{
	static const uint PLUS = 1,
	MINUS = 2,
	HOME = 3,
	LEFT = 4, RIGHT = 5, UP = 6, DOWN = 7,
	A = 8, B = 9,
	X = 10, Y = 11,
	L = 12, R = 13,
	ZL = 14, ZR = 15,
	LSTICK_LEFT = 16, LSTICK_RIGHT = 17, LSTICK_UP = 18, LSTICK_DOWN = 19,
	RSTICK_LEFT = 20, RSTICK_RIGHT = 21, RSTICK_UP = 22, RSTICK_DOWN = 23,
	LH = 24, RH = 25
	;

	static const uint COUNT = 26;
	}

	namespace iControlPad
	{
	static const uint A = 1,
	B = 2,
	X = 3,
	Y = 4,
	L = 5,
	R = 6,
	START = 7,
	SELECT = 8,
	LNUB_LEFT = 9, LNUB_RIGHT = 10, LNUB_UP = 11, LNUB_DOWN = 12,
	RNUB_LEFT = 13, RNUB_RIGHT = 14, RNUB_UP = 15, RNUB_DOWN = 16,
	LEFT = 17, RIGHT = 18, UP = 19, DOWN = 20
	;

	static const uint COUNT = 21;
	}

	namespace Zeemote
	{
	static const uint A = 1,
	B = 2,
	C = 3,
	POWER = 4,
	// Directions (from analog stick)
	LEFT = 5, RIGHT = 6, UP = 7, DOWN = 8
	;

	static const uint COUNT = 9;
	}

	namespace ICade
	{
	// Due to lack of naming standard and for clarity,
	// face buttons are labeled like a SEGA Saturn controller
	// with E1 and E2 as SELECT and START respectively

	static const uint UP = Keycode::UP,
	RIGHT = Keycode::RIGHT,
	DOWN = Keycode::DOWN,
	LEFT = Keycode::LEFT,
	X = Keycode::GAME_X,
	A = Keycode::GAME_A,
	Y = Keycode::GAME_Y,
	B = Keycode::GAME_B,
	Z = Keycode::GAME_Z,
	C = Keycode::GAME_C,
	SELECT = Keycode::GAME_SELECT,
	START = Keycode::GAME_START
	;

	static const uint COUNT = Keycode::COUNT;
	}

}
