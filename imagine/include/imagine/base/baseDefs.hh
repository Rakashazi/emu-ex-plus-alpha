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
#include <vector>
#include <memory>

namespace Config
{
#if defined __ANDROID__
#define CONFIG_BASE_SUPPORTS_VIBRATOR
static constexpr bool BASE_SUPPORTS_VIBRATOR = true;
#else
static constexpr bool BASE_SUPPORTS_VIBRATOR = false;
#endif

#if defined __ANDROID__ || (defined __APPLE__ && TARGET_OS_IPHONE)
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

#if defined __ANDROID__ || defined CONFIG_BASE_IOS
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

	namespace Base
	{
	#if defined __linux__
	#define CONFIG_BASE_GL_PLATFORM_EGL
	static constexpr bool GL_PLATFORM_EGL = true;
	#else
	static constexpr bool GL_PLATFORM_EGL = false;
	#endif
	}
}

namespace Input
{
class Event;
class Device;
class DeviceChange;
}

namespace Base
{
using namespace IG;

using OnFrameDelegate = DelegateFunc<bool (FrameParams params)>;

// orientation
using Orientation = uint8_t;
static constexpr Orientation VIEW_ROTATE_0 = bit(0), VIEW_ROTATE_90 = bit(1), VIEW_ROTATE_180 = bit(2), VIEW_ROTATE_270 = bit(3);
static constexpr Orientation VIEW_ROTATE_AUTO = bit(5);
static constexpr Orientation VIEW_ROTATE_ALL = VIEW_ROTATE_0 | VIEW_ROTATE_90 | VIEW_ROTATE_180 | VIEW_ROTATE_270;
static constexpr Orientation VIEW_ROTATE_ALL_BUT_UPSIDE_DOWN = VIEW_ROTATE_0 | VIEW_ROTATE_90 | VIEW_ROTATE_270;

const char *orientationToStr(Orientation o);
bool orientationIsSideways(Orientation o);

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
		CUSTOM_VIEWPORT_RESIZED = IG::bit(2);
	static constexpr uint8_t RESIZE_BITS =
		SURFACE_RESIZED | CONTENT_RECT_RESIZED | CUSTOM_VIEWPORT_RESIZED;

	constexpr WindowSurfaceChange(Action action, uint8_t flags = 0):
		action_{action}, flags{flags}
	{}

	constexpr Action action() const
	{
		return action_;
	}

	constexpr bool resized() const
	{
		return action() == Action::CHANGED;
	}

	constexpr bool surfaceResized() const { return flags & SURFACE_RESIZED; }
	constexpr bool contentRectResized() const { return flags & CONTENT_RECT_RESIZED; }
	constexpr bool customViewportResized() const { return flags & CUSTOM_VIEWPORT_RESIZED; }

protected:
	Action action_{};
	uint8_t flags{};
};

struct WindowDrawParams
{
	bool wasResized_ = false;
	bool needsSync_ = false;

	constexpr WindowDrawParams() {}
	bool wasResized() const { return wasResized_; }
	bool needsSync() const { return needsSync_; }
};

struct ScreenChange
{
	uint32_t state;
	enum { ADDED, REMOVED };

	constexpr ScreenChange(uint32_t state): state(state) {}
	bool added() const { return state == ADDED; }
	bool removed() const { return state == REMOVED; }
};

class Screen;
class Window;
class WindowConfig;
class ApplicationContext;

using WindowContainter = std::vector<std::unique_ptr<Window>>;
using ScreenContainter = std::vector<std::unique_ptr<Screen>>;
using InputDeviceContainer = std::vector<Input::Device*>;

using MainThreadMessageDelegate = DelegateFunc<void(ApplicationContext)>;
using InterProcessMessageDelegate = DelegateFunc<void (ApplicationContext, const char *filename)>;
using ResumeDelegate = DelegateFunc<bool (ApplicationContext, bool focused)>;
using FreeCachesDelegate = DelegateFunc<void (ApplicationContext, bool running)>;
using ExitDelegate = DelegateFunc<bool (ApplicationContext, bool backgrounded)>;
using DeviceOrientationChangedDelegate = DelegateFunc<void (ApplicationContext, Orientation newOrientation)>;
using SystemOrientationChangedDelegate = DelegateFunc<void (ApplicationContext, Orientation oldOrientation, Orientation newOrientation)>;
using ScreenChangeDelegate = DelegateFunc<void (ApplicationContext, Screen &s, ScreenChange)>;
using SystemPathPickerDelegate = DelegateFunc<void(const char *path)>;

using InputDeviceChangeDelegate = DelegateFunc<void (const Input::Device &dev, Input::DeviceChange)>;
using InputDevicesEnumeratedDelegate = DelegateFunc<void ()>;

using WindowInitDelegate = DelegateFunc<void (ApplicationContext, Window &)>;
using WindowInitDelegate = DelegateFunc<void (ApplicationContext, Window &)>;
using WindowSurfaceChangeDelegate = DelegateFunc<void (Window &, WindowSurfaceChange)>;
using WindowDrawDelegate = DelegateFunc<bool (Window &, WindowDrawParams)>;
using WindowInputEventDelegate = DelegateFunc<bool (Window &, Input::Event)>;
using WindowFocusChangeDelegate = DelegateFunc<void (Window &, bool in)>;
using WindowDragDropDelegate = DelegateFunc<void (Window &, const char *filename)>;
using WindowDismissRequestDelegate = DelegateFunc<void (Window &)>;
using WindowDismissDelegate = DelegateFunc<void (Window &)>;

using ScreenId = std::conditional_t<Config::envIsAndroid, int, void*>;
using NativeDisplayConnection = void*;

}
