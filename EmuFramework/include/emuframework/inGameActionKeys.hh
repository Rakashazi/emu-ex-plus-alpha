#pragma once

/*  This file is part of EmuFramework.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with EmuFramework.  If not, see <http://www.gnu.org/licenses/> */

namespace EmuControls
{

static const uint gameActionKeys = 9;
static const uint systemKeyMapStart = gameActionKeys;
typedef uint GameActionKeyArray[gameActionKeys];

static const char *gameActionName[gameActionKeys] =
{
	"Load Game",
	"Open Menu",
	"Save State",
	"Load State",
	"Decrement State Slot",
	"Increment State Slot",
	"Fast-forward",
	"Game Screenshot",
	"Exit",
};

}

#define EMU_CONTROLS_IN_GAME_ACTIONS_CATEGORY_INIT \
{"Set In-Game Actions", gameActionName, 0}

#define EMU_CONTROLS_IN_GAME_ACTIONS_UNBINDED_PROFILE_INIT \
0, 0, 0, 0, 0, 0, 0, 0, 0

#define EMU_CONTROLS_IN_GAME_ACTIONS_ICP_NUBS_PROFILE_INIT \
Input::iControlPad::RNUB_DOWN, \
Input::iControlPad::RNUB_UP, \
0, \
0, \
0, \
0, \
Input::iControlPad::LNUB_UP, \
0, \
0

#define EMU_CONTROLS_IN_GAME_ACTIONS_ICADE_PROFILE_INIT \
0, \
Input::ICade::Z, \
0, \
0, \
0, \
0, \
0, \
0, \
0

#define EMU_CONTROLS_IN_GAME_ACTIONS_WIIMOTE_PROFILE_INIT \
0, \
Input::Wiimote::HOME, \
0, \
0, \
0, \
0, \
0, \
0, \
0

#define EMU_CONTROLS_IN_GAME_ACTIONS_WII_CC_PROFILE_INIT \
0, \
Input::WiiCC::HOME, \
0, \
0, \
0, \
0, \
Input::WiiCC::ZR, \
0, \
0

#define EMU_CONTROLS_IN_GAME_ACTIONS_WEBOS_KB_PROFILE_INIT \
Input::Keycode::LSHIFT, \
Input::Keycode::RCTRL, \
Input::Keycode::Q, \
Input::Keycode::A, \
0, \
0, \
Input::Keycode::AT, \
0, \
0

#define EMU_CONTROLS_WEBOS_KB_8WAY_DIRECTION_PROFILE_INIT \
Input::Keycode::R, \
Input::Keycode::G, \
Input::Keycode::C, \
Input::Keycode::D, \
Input::Keycode::E, \
Input::Keycode::T, \
Input::Keycode::V, \
Input::Keycode::X

#define EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_NAV_PROFILE_INIT \
0, \
Input::Keycode::MENU, \
0, \
0, \
0, \
0, \
Input::Keycode::SEARCH, \
0, \
Input::Keycode::BACK

#define EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_GENERIC_GAMEPAD_PROFILE_INIT \
0, \
Input::Keycode::JS2_YAXIS_NEG, \
0, \
0, \
0, \
0, \
Input::Keycode::JS_RTRIGGER_AXIS, \
0, \
0

#define EMU_CONTROLS_IN_GAME_ACTIONS_OUYA_PROFILE_INIT \
0, \
Input::Keycode::MENU, \
0, \
0, \
0, \
0, \
Input::Keycode::Ouya::R2, \
0, \
0

#define EMU_CONTROLS_IN_GAME_ACTIONS_OUYA_MINIMAL_PROFILE_INIT \
0, \
Input::Keycode::MENU, \
0, \
0, \
0, \
0, \
0, \
0, \
0

#define EMU_CONTROLS_IN_GAME_ACTIONS_NVIDIA_SHIELD_PROFILE_INIT \
0, \
0, \
0, \
0, \
0, \
0, \
Input::Keycode::JS_RTRIGGER_AXIS, \
0, \
Input::Keycode::BACK

#define EMU_CONTROLS_IN_GAME_ACTIONS_NVIDIA_SHIELD_MINIMAL_PROFILE_INIT \
0, \
0, \
0, \
0, \
0, \
0, \
Input::Keycode::JS_RTRIGGER_AXIS, \
0, \
Input::Keycode::BACK

#define EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_PS3_GAMEPAD_PROFILE_INIT \
0, \
Input::Keycode::GAME_1, \
0, \
0, \
0, \
0, \
Input::Keycode::GAME_R2, \
0, \
0

#define EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_PS3_GAMEPAD_MINIMAL_PROFILE_INIT \
0, \
Input::Keycode::GAME_1, \
0, \
0, \
0, \
0, \
0, \
0, \
0

#define EMU_CONTROLS_IN_GAME_ACTIONS_GENERIC_KB_PROFILE_INIT \
Input::Keycode::L, \
Input::Keycode::MENU, \
Input::Keycode::F1, \
Input::Keycode::F4, \
Input::Keycode::LEFT_BRACKET, \
Input::Keycode::RIGHT_BRACKET, \
Input::Keycode::GRAVE, \
0, \
Input::Keycode::ESCAPE

#define EMU_CONTROLS_IN_GAME_ACTIONS_GENERIC_KB_ALT_PROFILE_INIT \
Input::Keycode::L, \
Input::Keycode::MENU, \
Input::Keycode::F9, \
Input::Keycode::F12, \
Input::Keycode::LEFT_BRACKET, \
Input::Keycode::RIGHT_BRACKET, \
Input::Keycode::GRAVE, \
0, \
Input::Keycode::ESCAPE

#ifdef CONFIG_BASE_ANDROID
#define EMU_CONTROLS_IN_GAME_ACTIONS_GENERIC_KB_MINIMAL_PROFILE_INIT \
0, \
Input::Keycode::MENU, \
0, \
0, \
0, \
0, \
Input::Keycode::SEARCH, \
0, \
0
#else
#define EMU_CONTROLS_IN_GAME_ACTIONS_GENERIC_KB_MINIMAL_PROFILE_INIT \
0, \
Input::Keycode::MENU, \
0, \
0, \
0, \
0, \
Input::Keycode::F11, \
0, \
0
#endif

#ifdef CONFIG_BASE_PS3
#define PS3PAD_OPEN_MENU_KEY Input::PS3::L2
#else
#define PS3PAD_OPEN_MENU_KEY Input::PS3::PS
#endif

#define EMU_CONTROLS_IN_GAME_ACTIONS_GENERIC_PS3PAD_PROFILE_INIT \
	0, \
	PS3PAD_OPEN_MENU_KEY, \
	0, \
	0, \
	0, \
	0, \
	Input::PS3::R2, \
	0, \
	0

#define EMU_CONTROLS_IN_GAME_ACTIONS_GENERIC_PS3PAD_ALT_MINIMAL_PROFILE_INIT \
	0, \
	Input::PS3::SELECT, \
	0, \
	0, \
	0, \
	0, \
	0, \
	0, \
	0

#define EMU_CONTROLS_IN_GAME_ACTIONS_PANDORA_PROFILE_INIT \
	Input::Keycode::L, \
	Input::Keycode::SPACE, \
	Input::Keycode::_3, \
	Input::Keycode::_4, \
	Input::Keycode::_5, \
	Input::Keycode::_6, \
	Input::Keycode::Pandora::R, \
	0, \
	Input::Keycode::BACK_SPACE

#define EMU_CONTROLS_IN_GAME_ACTIONS_PANDORA_ALT_PROFILE_INIT \
	Input::Keycode::L, \
	Input::Keycode::SPACE, \
	Input::Keycode::_3, \
	Input::Keycode::_4, \
	Input::Keycode::_5, \
	Input::Keycode::_6, \
	Input::Keycode::_0, \
	0, \
	Input::Keycode::BACK_SPACE

#define EMU_CONTROLS_IN_GAME_ACTIONS_PANDORA_ALT_MINIMAL_PROFILE_INIT \
	0, \
	Input::Keycode::Pandora::START, \
	0, \
	0, \
	0, \
	0, \
	Input::Keycode::Pandora::R, \
	0, \
	0

#define EMU_CONTROLS_IN_GAME_ACTIONS_APPLEGC_PROFILE_INIT \
	0, \
	Input::AppleGC::PAUSE, \
	0, \
	0, \
	0, \
	0, \
	Input::AppleGC::R2, \
	0, \
	0

#define EMU_CONTROLS_IN_GAME_ACTIONS_APPLEGC_MINIMAL_PROFILE_INIT \
	0, \
	Input::AppleGC::PAUSE, \
	0, \
	0, \
	0, \
	0, \
	0, \
	0, \
	0
