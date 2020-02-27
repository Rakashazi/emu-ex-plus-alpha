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

#include <imagine/config/defs.hh>
#include <imagine/util/DelegateFunc.hh>
#include <imagine/util/bits.h>
#include <imagine/time/Time.hh>

namespace Config
{
#if defined __ANDROID__ && !defined CONFIG_MACHINE_OUYA
#define CONFIG_BASE_SUPPORTS_VIBRATOR
static constexpr bool BASE_SUPPORTS_VIBRATOR = true;
#else
static constexpr bool BASE_SUPPORTS_VIBRATOR = false;
#endif

#if defined __ANDROID__ || (defined __APPLE__ && TARGET_OS_IPHONE) || defined CONFIG_ENV_WEBOS
#define CONFIG_BASE_CAN_BACKGROUND_APP
static constexpr bool BASE_CAN_BACKGROUND_APP = true;
#else
static constexpr bool BASE_CAN_BACKGROUND_APP = false;
#endif

#if defined __ANDROID__ || (defined __APPLE__ && TARGET_OS_IPHONE)
#define CONFIG_BASE_SUPPORTS_ORIENTATION_SENSOR
static constexpr bool BASE_SUPPORTS_ORIENTATION_SENSOR = true;
#else
static constexpr bool BASE_SUPPORTS_ORIENTATION_SENSOR = false;
#endif

#if (defined __ANDROID__ && !(defined __arm__ && __ARM_ARCH < 7)) || defined CONFIG_BASE_IOS
#define CONFIG_BASE_MULTI_SCREEN
#define CONFIG_BASE_SCREEN_HOTPLUG
static constexpr bool BASE_MULTI_SCREEN = true;
#else
static constexpr bool BASE_MULTI_SCREEN = false;
#endif

#if defined CONFIG_BASE_IOS
#define CONFIG_BASE_SCREEN_FRAME_INTERVAL
#endif

#if (defined CONFIG_BASE_X11 && !defined CONFIG_MACHINE_PANDORA) || defined CONFIG_BASE_MULTI_SCREEN
#define CONFIG_BASE_MULTI_WINDOW
static constexpr bool BASE_MULTI_WINDOW = true;
#else
static constexpr bool BASE_MULTI_WINDOW = false;
#endif

#if defined CONFIG_BASE_IOS && defined __ARM_ARCH_6K__
#define CONFIG_GFX_SOFT_ORIENTATION 1
#elif !defined __ANDROID__ && !defined CONFIG_BASE_IOS
#define CONFIG_GFX_SOFT_ORIENTATION 1
#endif

#if defined CONFIG_GFX_SOFT_ORIENTATION
static constexpr bool SYSTEM_ROTATES_WINDOWS = false;
#else
static constexpr bool SYSTEM_ROTATES_WINDOWS = true;
#endif
}

namespace Base
{
using namespace IG;

#if defined __APPLE__ && TARGET_OS_IPHONE
using FrameTime = FloatSeconds;
#else
using FrameTime = Nanoseconds;
#endif

// orientation
using Orientation = uint32_t;
static constexpr Orientation VIEW_ROTATE_0 = bit(0), VIEW_ROTATE_90 = bit(1), VIEW_ROTATE_180 = bit(2), VIEW_ROTATE_270 = bit(3);
static constexpr Orientation VIEW_ROTATE_AUTO = bit(5);
static constexpr Orientation VIEW_ROTATE_ALL = VIEW_ROTATE_0 | VIEW_ROTATE_90 | VIEW_ROTATE_180 | VIEW_ROTATE_270;
static constexpr Orientation VIEW_ROTATE_ALL_BUT_UPSIDE_DOWN = VIEW_ROTATE_0 | VIEW_ROTATE_90 | VIEW_ROTATE_270;

const char *orientationToStr(Orientation o);
bool orientationIsSideways(Orientation o);
Orientation validateOrientationMask(Orientation oMask);

// App callback types

using InterProcessMessageDelegate = DelegateFunc<void (const char *filename)>;
using ResumeDelegate = DelegateFunc<bool (bool focused)>;
using FreeCachesDelegate = DelegateFunc<void ()>;
using ExitDelegate = DelegateFunc<bool (bool backgrounded)>;
using DeviceOrientationChangedDelegate = DelegateFunc<void (Orientation newOrientation)>;
using SystemOrientationChangedDelegate = DelegateFunc<void (Orientation oldOrientation, Orientation newOrientation)>;
}
