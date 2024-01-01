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

#include <imagine/input/inputDefs.hh>

namespace IG::Input
{
	namespace PS3Key
	{
	constexpr Key CROSS = Keycode::GAME_A,
	CIRCLE = Keycode::GAME_B,
	SQUARE = Keycode::GAME_X,
	TRIANGLE = Keycode::GAME_Y,
	L1 = Keycode::GAME_L1,
	L2 = Keycode::GAME_L2,
	L3 = Keycode::GAME_LEFT_THUMB,
	R1 = Keycode::GAME_R1,
	R2 = Keycode::GAME_R2,
	R3 = Keycode::GAME_RIGHT_THUMB,
	SELECT = Keycode::GAME_SELECT,
	START = Keycode::GAME_START,
	UP =   Keycode::UP,   RIGHT = Keycode::RIGHT,
	DOWN = Keycode::DOWN, LEFT = Keycode::LEFT,
	PS = Keycode::GAME_MODE,
	LSTICK_RIGHT = Keycode::JS1_XAXIS_POS, LSTICK_LEFT = Keycode::JS1_XAXIS_NEG,
	LSTICK_DOWN =  Keycode::JS1_YAXIS_POS, LSTICK_UP =   Keycode::JS1_YAXIS_NEG,
	RSTICK_RIGHT = Keycode::JS2_XAXIS_POS, RSTICK_LEFT = Keycode::JS2_XAXIS_NEG,
	RSTICK_DOWN =  Keycode::JS2_YAXIS_POS, RSTICK_UP =   Keycode::JS2_YAXIS_NEG
	;
	}

	namespace WiimoteKey
	{
	constexpr Key PLUS = Keycode::GAME_START,
	MINUS = Keycode::GAME_SELECT,
	HOME = Keycode::GAME_MODE,
	LEFT = Keycode::LEFT, RIGHT = Keycode::RIGHT,
	UP = Keycode::UP, DOWN = Keycode::DOWN,
	_1 = Keycode::GAME_X,
	_2 = Keycode::GAME_A,
	A = Keycode::GAME_Y,
	B = Keycode::GAME_B,
	// Nunchuk
	NUN_C = Keycode::GAME_Z, NUN_Z = Keycode::GAME_C,
	NUN_STICK_LEFT = Keycode::JS1_XAXIS_NEG, NUN_STICK_RIGHT = Keycode::JS1_XAXIS_POS,
	NUN_STICK_UP = Keycode::JS1_YAXIS_NEG, NUN_STICK_DOWN = Keycode::JS1_YAXIS_POS
	;
	}

	namespace WiiCCKey
	{
	constexpr Key PLUS = Keycode::GAME_START,
	MINUS = Keycode::GAME_SELECT,
	HOME = Keycode::GAME_MODE,
	LEFT = Keycode::LEFT, RIGHT = Keycode::RIGHT,
	UP = Keycode::UP, DOWN = Keycode::DOWN,
	A = Keycode::GAME_B, B = Keycode::GAME_A,
	X = Keycode::GAME_Y, Y = Keycode::GAME_X,
	L = Keycode::GAME_L1, R = Keycode::GAME_R1,
	ZL = Keycode::GAME_L2, ZR = Keycode::GAME_R2,
	LSTICK_RIGHT = Keycode::JS1_XAXIS_POS, LSTICK_LEFT = Keycode::JS1_XAXIS_NEG,
	LSTICK_DOWN =  Keycode::JS1_YAXIS_POS, LSTICK_UP =   Keycode::JS1_YAXIS_NEG,
	RSTICK_RIGHT = Keycode::JS2_XAXIS_POS, RSTICK_LEFT = Keycode::JS2_XAXIS_NEG,
	RSTICK_DOWN =  Keycode::JS2_YAXIS_POS, RSTICK_UP =   Keycode::JS2_YAXIS_NEG,
	LH = Keycode::GAME_LEFT_THUMB, RH = Keycode::GAME_RIGHT_THUMB
	;
	}

	namespace iControlPadKey
	{
	constexpr Key A = Keycode::GAME_X,
	B = Keycode::GAME_B,
	X = Keycode::GAME_A,
	Y = Keycode::GAME_Y,
	L = Keycode::GAME_L1,
	R = Keycode::GAME_R1,
	START = Keycode::GAME_START,
	SELECT = Keycode::GAME_SELECT,
	LNUB_LEFT = Keycode::JS1_XAXIS_NEG, LNUB_RIGHT = Keycode::JS1_XAXIS_POS,
	LNUB_UP = Keycode::JS1_YAXIS_NEG, LNUB_DOWN = Keycode::JS1_YAXIS_POS,
	RNUB_LEFT = Keycode::JS2_XAXIS_NEG, RNUB_RIGHT = Keycode::JS2_XAXIS_POS,
	RNUB_UP = Keycode::JS2_YAXIS_NEG, RNUB_DOWN = Keycode::JS2_YAXIS_POS,
	LEFT = Keycode::LEFT, RIGHT = Keycode::RIGHT,
	UP = Keycode::UP, DOWN = Keycode::DOWN
	;
	}

	namespace ZeemoteKey
	{
	constexpr Key A = Keycode::GAME_X,
	B = Keycode::GAME_A,
	C = Keycode::GAME_B,
	POWER = Keycode::GAME_Y,
	// Directions (from analog stick)
	LEFT = Keycode::LEFT, RIGHT = Keycode::RIGHT,
	UP = Keycode::UP, DOWN = Keycode::DOWN
	;
	}

}
