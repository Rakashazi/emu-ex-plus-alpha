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
#include <imagine/time/Time.hh>
#include <imagine/util/DelegateFunc.hh>
#include <imagine/util/bitset.hh>
#include <imagine/util/string/CStringView.hh>
#include <imagine/util/enum.hh>
#include <vector>
#include <memory>

namespace Config
{
#if defined __ANDROID__
static constexpr bool BASE_SUPPORTS_VIBRATOR = true;
#else
static constexpr bool BASE_SUPPORTS_VIBRATOR = false;
#endif

#if defined __ANDROID__ || (defined __APPLE__ && TARGET_OS_IPHONE)
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

#if defined __ANDROID__ || defined CONFIG_BASE_IOS
#define CONFIG_BASE_MULTI_SCREEN
static constexpr bool BASE_MULTI_SCREEN = true;
#else
static constexpr bool BASE_MULTI_SCREEN = false;
#endif

#if defined CONFIG_BASE_IOS
#define CONFIG_BASE_SCREEN_FRAME_INTERVAL
static constexpr bool SCREEN_FRAME_INTERVAL = true;
#else
static constexpr bool SCREEN_FRAME_INTERVAL = false;
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

#if defined __linux__
#define CONFIG_BASE_GL_PLATFORM_EGL
static constexpr bool GL_PLATFORM_EGL = true;
#else
static constexpr bool GL_PLATFORM_EGL = false;
#endif
static constexpr bool SYSTEM_FILE_PICKER = Config::envIsAndroid;

#if defined __ANDROID__ || (defined __APPLE__ && TARGET_OS_IPHONE)
#define CONFIG_BASE_STATUS_BAR
static constexpr bool STATUS_BAR = true;
#else
static constexpr bool STATUS_BAR = false;
#endif

#if defined __ANDROID__
#define CONFIG_BASE_NAVIGATION_BAR
static constexpr bool NAVIGATION_BAR = true;
#else
static constexpr bool NAVIGATION_BAR = false;
#endif

#if defined __ANDROID__
constexpr bool TRANSLUCENT_SYSTEM_UI = true;
#else
constexpr bool TRANSLUCENT_SYSTEM_UI = false;
#endif

#if defined __ANDROID__
constexpr bool DISPLAY_CUTOUT = true;
#else
constexpr bool DISPLAY_CUTOUT = false;
#endif

#if defined __ANDROID__
#define IG_CONFIG_SENSORS
constexpr bool SENSORS = true;
#else
constexpr bool SENSORS = false;
#endif

}

namespace IG::Input
{
class Event;
class Device;
class DeviceChange;
}

namespace IG
{

using OnFrameDelegate = DelegateFunc<bool (FrameParams params)>;


enum class OrientationMask: uint8_t
{
	UNSET,
	PORTRAIT = bit(0),
	LANDSCAPE_RIGHT = bit(1),
	PORTRAIT_UPSIDE_DOWN = bit(2),
	LANDSCAPE_LEFT = bit(3),
	ALL_LANDSCAPE = LANDSCAPE_RIGHT | LANDSCAPE_LEFT,
	ALL_PORTRAIT = PORTRAIT | PORTRAIT_UPSIDE_DOWN,
	ALL_BUT_UPSIDE_DOWN = PORTRAIT | LANDSCAPE_RIGHT | LANDSCAPE_LEFT,
	ALL = PORTRAIT | LANDSCAPE_RIGHT | PORTRAIT_UPSIDE_DOWN | LANDSCAPE_LEFT,
};

IG_DEFINE_ENUM_BIT_FLAG_FUNCTIONS(OrientationMask);

std::string_view asString(OrientationMask);

WISE_ENUM_CLASS((Rotation, uint8_t),
	UP,
	RIGHT,
	DOWN,
	LEFT,
	ANY);

constexpr bool isSideways(Rotation r) { return r == Rotation::LEFT || r == Rotation::RIGHT; }

static constexpr int APP_ON_EXIT_PRIORITY = 0;
static constexpr int RENDERER_TASK_ON_EXIT_PRIORITY = 200;
static constexpr int RENDERER_DRAWABLE_ON_EXIT_PRIORITY = 300;
static constexpr int WINDOW_ON_EXIT_PRIORITY = 400;
static constexpr int SCREEN_ON_EXIT_PRIORITY = 500;
static constexpr int INPUT_DEVICE_ON_EXIT_PRIORITY = 600;

static constexpr int INPUT_DEVICE_ON_RESUME_PRIORITY = -INPUT_DEVICE_ON_EXIT_PRIORITY;
static constexpr int SCREEN_ON_RESUME_PRIORITY = -SCREEN_ON_EXIT_PRIORITY;
static constexpr int WINDOW_ON_RESUME_PRIORITY = -WINDOW_ON_EXIT_PRIORITY;
static constexpr int RENDERER_DRAWABLE_ON_RESUME_PRIORITY = -RENDERER_DRAWABLE_ON_EXIT_PRIORITY;
static constexpr int RENDERER_TASK_ON_RESUME_PRIORITY = -RENDERER_TASK_ON_EXIT_PRIORITY;
static constexpr int APP_ON_RESUME_PRIORITY = 0;

// Window/Screen helper classes

struct WindowSurfaceChange
{
	enum class Action : uint8_t
	{
		CREATED, CHANGED, DESTROYED
	};

	static constexpr uint8_t SURFACE_RESIZED = IG::bit(0),
		CONTENT_RECT_RESIZED = IG::bit(1),
		RESIZE_BITS = SURFACE_RESIZED | CONTENT_RECT_RESIZED;

	Action action{};
	uint8_t flags{};

	constexpr WindowSurfaceChange(Action action, uint8_t flags = 0):
		action{action}, flags{flags} {}
	constexpr bool resized() const { return action == Action::CHANGED; }
	constexpr bool surfaceResized() const { return flags & SURFACE_RESIZED; }
	constexpr bool contentRectResized() const { return flags & CONTENT_RECT_RESIZED; }
};

struct WindowDrawParams
{
	bool wasResized{};
	bool needsSync{};
};

enum class WindowFrameTimeSource : uint8_t
{
	AUTO,
	SCREEN,
	RENDERER,
};

struct ScreenChange
{
	enum class Action : int8_t
	{
		ADDED,
		REMOVED
	};

	Action action{};

	constexpr ScreenChange(Action action): action{action} {}
	constexpr bool added() const { return action == Action::ADDED; }
	constexpr bool removed() const { return action == Action::REMOVED; }
};

WISE_ENUM_CLASS((SensorType, uint8_t),
	(Accelerometer, 1),
	(Gyroscope, 4)
);

using SensorValues = std::array<float, 3>;

class Screen;
class Window;
class WindowConfig;
class ApplicationContext;

using WindowContainer = std::vector<std::unique_ptr<Window>>;
using ScreenContainer = std::vector<std::unique_ptr<Screen>>;
using InputDeviceContainer = std::vector<std::unique_ptr<Input::Device>>;

using MainThreadMessageDelegate = DelegateFunc<void(ApplicationContext)>;
using InterProcessMessageDelegate = DelegateFunc<void (ApplicationContext, IG::CStringView filename)>;
using ResumeDelegate = DelegateFunc<bool (ApplicationContext, bool focused)>;
using FreeCachesDelegate = DelegateFunc<void (ApplicationContext, bool running)>;
using ExitDelegate = DelegateFunc<bool (ApplicationContext, bool backgrounded)>;
using DeviceOrientationChangedDelegate = DelegateFunc<void (ApplicationContext, Rotation newRotation)>;
using SystemOrientationChangedDelegate = DelegateFunc<void (ApplicationContext, Rotation oldRotation, Rotation newRotation)>;
using ScreenChangeDelegate = DelegateFunc<void (ApplicationContext, Screen &s, ScreenChange)>;
using SystemDocumentPickerDelegate = DelegateFunc<void(IG::CStringView uri, IG::CStringView displayName)>;
using TextFieldDelegate = DelegateFunc<void (const char *str)>;
using SensorChangedDelegate = DelegateFunc<void (SensorValues)>;

using InputDeviceChangeDelegate = DelegateFunc<void (const Input::Device &dev, Input::DeviceChange)>;
using InputDevicesEnumeratedDelegate = DelegateFunc<void ()>;

using WindowInitDelegate = DelegateFunc<void (ApplicationContext, Window &)>;
using WindowInitDelegate = DelegateFunc<void (ApplicationContext, Window &)>;
using WindowSurfaceChangeDelegate = DelegateFunc<void (Window &, WindowSurfaceChange)>;
using WindowDrawDelegate = DelegateFunc<bool (Window &, WindowDrawParams)>;
using WindowInputEventDelegate = DelegateFunc<bool (Window &, const Input::Event &)>;
using WindowFocusChangeDelegate = DelegateFunc<void (Window &, bool in)>;
using WindowDragDropDelegate = DelegateFunc<void (Window &, IG::CStringView filename)>;
using WindowDismissRequestDelegate = DelegateFunc<void (Window &)>;
using WindowDismissDelegate = DelegateFunc<void (Window &)>;

using ScreenId = std::conditional_t<Config::envIsAndroid, int, void*>;
using NativeDisplayConnection = void*;

}
