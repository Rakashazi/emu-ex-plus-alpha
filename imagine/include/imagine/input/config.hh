#pragma once

#include <imagine/engine-globals.h>

#ifdef CONFIG_BASE_X11
#include <imagine/base/x11/inputDefs.hh>
#elif defined __ANDROID__
#include <imagine/base/android/inputDefs.hh>
#elif defined CONFIG_BASE_SDL
#include <imagine/base/sdl/inputDefs.hh>
#elif defined CONFIG_BASE_IOS
#include <imagine/base/iphone/inputDefs.hh>
#elif defined CONFIG_BASE_MACOSX
#include <imagine/base/osx/inputDefs.hh>
#elif defined CONFIG_BASE_WIN32
#include <imagine/base/win32/inputDefs.hh>
#elif defined CONFIG_BASE_PS3
#include <imagine/input/ps3/inputDefs.hh>
#endif

#ifdef CONFIG_INPUT_EVDEV
#include <imagine/input/evdev/inputDefs.hh>
#endif

namespace Config
{

	namespace Input
	{

	#if (defined __APPLE__ && TARGET_OS_IPHONE) || defined __ANDROID__
	#define CONFIG_INPUT_SYSTEM_COLLECTS_TEXT
	static constexpr bool SYSTEM_COLLECTS_TEXT = true;
	#else
	static constexpr bool SYSTEM_COLLECTS_TEXT = false;
	#endif

	// dynamic input device list from system
	#if defined CONFIG_BASE_X11 || defined __ANDROID__ || defined __APPLE__
	#define CONFIG_INPUT_DEVICE_HOTSWAP
	static constexpr bool DEVICE_HOTSWAP = true;
	#else
	static constexpr bool DEVICE_HOTSWAP = false;
	#endif

	#define CONFIG_INPUT_KEYBOARD_DEVICES
	static constexpr bool KEYBOARD_DEVICES = true;

	// mouse & touch
	#define CONFIG_INPUT_POINTING_DEVICES
	static constexpr bool POINTING_DEVICES = true;

	#if defined CONFIG_BASE_X11 || defined __ANDROID__ || defined _WIN32
	#define CONFIG_INPUT_MOUSE_DEVICES
	static constexpr bool MOUSE_DEVICES = true;
	#else
	static constexpr bool MOUSE_DEVICES = false;
	#endif

	#if defined CONFIG_BASE_X11 || defined __ANDROID__ || defined _WIN32
	#define CONFIG_INPUT_GAMEPAD_DEVICES
	static constexpr bool GAMEPAD_DEVICES = true;
	#else
	static constexpr bool GAMEPAD_DEVICES = false;
	#endif

	#if (defined __APPLE__ && TARGET_OS_IPHONE) || defined __ANDROID__
	#define CONFIG_INPUT_TOUCH_DEVICES
	static constexpr bool TOUCH_DEVICES = true;
	#else
	static constexpr bool TOUCH_DEVICES = false;
	#endif

	static constexpr uint8 MAX_POINTERS =
	#if defined CONFIG_BASE_X11
	4; // arbitrary max
	#elif defined CONFIG_BASE_IOS || defined __ANDROID__
	// arbitrary max
	Config::MACHINE_IS_GENERIC_ARMV6 ? 4 : 7;
	#elif defined CONFIG_ENV_WEBOS
	4; // max 5
	#else
	1;
	#endif

	// relative motion/trackballs
	#ifdef __ANDROID__
	#define CONFIG_INPUT_RELATIVE_MOTION_DEVICES
	static constexpr bool RELATIVE_MOTION_DEVICES = true;
	#else
	static constexpr bool RELATIVE_MOTION_DEVICES = false;
	#endif

	}

}

#ifdef CONFIG_BLUETOOTH
#include <imagine/input/bluetoothInputDefs.hh>
#endif
