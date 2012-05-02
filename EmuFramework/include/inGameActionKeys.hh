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

#pragma once

namespace EmuControls
{

static const uint gameActionKeys = 7;
static const uint systemKeyMapStart = gameActionKeys;
typedef uint GameActionKeyArray[gameActionKeys];

static const char *gameActionName[gameActionKeys] =
{
	"Load Game",
	"Open Menu",
	"Save State",
	"Load State",
	"Fast-forward",
	"Game Screenshot",
	"Exit",
};

}

#define EMU_CONTROLS_IN_GAME_ACTIONS_CATEGORY_INIT \
KeyCategory("In-Game Actions", gameActionName, 0)

#define EMU_CONTROLS_IN_GAME_ACTIONS_UNBINDED_PROFILE_INIT \
0, 0, 0, 0, 0, 0, 0

#define EMU_CONTROLS_IN_GAME_ACTIONS_ICP_NUBS_PROFILE_INIT \
Input::iControlPad::RNUB_DOWN, \
Input::iControlPad::RNUB_UP, \
0, \
0, \
Input::iControlPad::LNUB_UP, \
0, \
0

#define EMU_CONTROLS_IN_GAME_ACTIONS_WIIMOTE_PROFILE_INIT \
0, \
Input::Wiimote::HOME, \
0, \
0, \
0, \
0, \
0

#define EMU_CONTROLS_IN_GAME_ACTIONS_WII_CC_PROFILE_INIT \
0, \
Input::Wiimote::HOME, \
0, \
0, \
Input::Wiimote::ZR, \
0, \
0

#define EMU_CONTROLS_IN_GAME_ACTIONS_WEBOS_KB_PROFILE_INIT \
Input::Key::LSHIFT, \
Input::Key::RCTRL, \
input_asciiKey('q'), \
input_asciiKey('a'), \
input_asciiKey('@'), \
0, \
0

#define EMU_CONTROLS_WEBOS_KB_8WAY_DIRECTION_PROFILE_INIT \
input_asciiKey('r'), \
input_asciiKey('g'), \
input_asciiKey('c'), \
input_asciiKey('d'), \
input_asciiKey('e'), \
input_asciiKey('t'), \
input_asciiKey('v'), \
input_asciiKey('x')

#define EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_NAV_PROFILE_INIT \
0, \
Input::Key::MENU, \
0, \
0, \
Input::Key::SEARCH, \
0, \
Input::Key::ESCAPE

#define EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_NAV_NO_BACK_PROFILE_INIT \
0, \
Input::Key::MENU, \
0, \
0, \
Input::Key::SEARCH, \
0, \
0

#define EMU_CONTROLS_IN_GAME_ACTIONS_GENERIC_KB_PROFILE_INIT \
input_asciiKey('l'), \
Input::Key::MENU, \
input_asciiKey('o'), \
input_asciiKey('p'), \
input_asciiKey('`'), \
0, \
Input::Key::ESCAPE

#define EMU_CONTROLS_IN_GAME_ACTIONS_GENERIC_PS3PAD_PROFILE_INIT \
	Input::Ps3::L1, \
	Input::Ps3::L2, \
	Input::Ps3::L3, \
	Input::Ps3::R3, \
	Input::Ps3::R2, \
	0, \
	0
