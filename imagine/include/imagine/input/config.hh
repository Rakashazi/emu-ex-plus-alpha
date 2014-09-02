#pragma once

#include <imagine/engine-globals.h>

#ifdef CONFIG_BASE_X11
#include <imagine/base/x11/inputDefs.hh>
#elif defined(CONFIG_BASE_ANDROID)
#include <imagine/input/android/inputDefs.hh>
#elif defined(CONFIG_BASE_SDL)
#include <imagine/base/sdl/inputDefs.hh>
#elif defined(CONFIG_BASE_IOS)
#include <imagine/base/iphone/inputDefs.hh>
#elif defined(CONFIG_BASE_MACOSX)
#include <imagine/base/osx/inputDefs.hh>
#elif defined(CONFIG_BASE_WIN32)
#include <imagine/base/win32/inputDefs.hh>
#elif defined(CONFIG_BASE_PS3)
#include <imagine/input/ps3/inputDefs.hh>
#endif

#ifdef CONFIG_INPUT_EVDEV
#include <imagine/input/evdev/inputDefs.hh>
#endif

namespace Input
{
// mouse/pointer/touch support
#ifndef CONFIG_INPUT_PS3
static constexpr bool SUPPORTS_POINTER = 1;
#define INPUT_SUPPORTS_POINTER
#else
static constexpr bool SUPPORTS_POINTER = 0;
#endif

#if defined CONFIG_BASE_X11 || defined CONFIG_BASE_WIN32
static constexpr bool SUPPORTS_MOUSE = 1;
#define INPUT_SUPPORTS_MOUSE
#else
static constexpr bool SUPPORTS_MOUSE = 0;
#endif

static constexpr uint8 maxCursors =
	#if defined CONFIG_BASE_X11
	4; // arbitrary max
	#elif defined CONFIG_BASE_IOS || defined __ANDROID__
		// arbitrary max
		#ifdef CONFIG_MACHINE_GENERIC_ARM
		4;
		#else
		7;
		#endif
	#elif defined CONFIG_ENV_WEBOS
	4; // max 5
	#else
	1;
	#endif

// relative pointer/trackball support
#ifdef CONFIG_BASE_ANDROID
static constexpr bool supportsRelativePointer = 1;
#define INPUT_SUPPORTS_RELATIVE_POINTER
#else
static constexpr bool supportsRelativePointer = 0;
#endif

// keyboard/key-based input support
#if !defined(CONFIG_BASE_IOS) && !defined(CONFIG_BASE_PS3)
static constexpr bool supportsKeyboard = 1;
#define INPUT_SUPPORTS_KEYBOARD
#else
static constexpr bool supportsKeyboard = 0;
#endif

// dynamic input device list from system
#if defined CONFIG_BASE_X11 || defined CONFIG_BASE_ANDROID || defined CONFIG_BASE_IOS
static constexpr bool hasSystemDeviceHotswap = 1;
#define INPUT_HAS_SYSTEM_DEVICE_HOTSWAP
#else
static constexpr bool hasSystemDeviceHotswap = 0;
#endif
}

#ifdef CONFIG_BLUETOOTH
#include <imagine/input/bluetoothInputDefs.hh>
#endif
