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

#include <imagine/config/defs.hh>
#include <imagine/bluetooth/defs.hh>

#ifdef ENV_NOTE
#define PLATFORM_INFO_STR ENV_NOTE " (" CONFIG_ARCH_STR ")"
#else
#define PLATFORM_INFO_STR "(" CONFIG_ARCH_STR ")"
#endif
#define CREDITS_INFO_STRING "Built : " __DATE__ "\n" PLATFORM_INFO_STR "\n\n"

namespace EmuEx
{

class EmuApp;
class EmuSystem;

#if defined __ANDROID__ || \
	defined CONFIG_OS_IOS || \
	(defined CONFIG_BASE_X11 && !defined CONFIG_MACHINE_PANDORA)
#define CONFIG_VCONTROLS_GAMEPAD
constexpr bool VCONTROLS_GAMEPAD = true;
#else
constexpr bool VCONTROLS_GAMEPAD = false;
#endif

constexpr bool HAS_MULTIPLE_WINDOW_PIXEL_FORMATS = Config::envIsLinux || Config::envIsAndroid || Config::envIsIOS;
constexpr bool MOGA_INPUT = Config::envIsAndroid;
constexpr bool CAN_HIDE_TITLE_BAR = !Config::envIsIOS;
constexpr bool enableFrameTimeStats = Config::DEBUG_BUILD;
constexpr bool hasICadeInput = Config::Input::KEYBOARD_DEVICES;

}
