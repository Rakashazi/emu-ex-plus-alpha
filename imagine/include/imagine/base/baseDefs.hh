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
#include <imagine/util/string/CStringView.hh>
#include <imagine/util/enum.hh>
#include <imagine/util/variant.hh>
#include <vector>
#include <memory>
#include <string_view>
#include <type_traits>
#include <variant>

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

#if defined __ANDROID__ || defined CONFIG_OS_IOS
#define CONFIG_BASE_MULTI_SCREEN
static constexpr bool BASE_MULTI_SCREEN = true;
#else
static constexpr bool BASE_MULTI_SCREEN = false;
#endif

#if defined CONFIG_OS_IOS
#define CONFIG_BASE_SCREEN_FRAME_INTERVAL
static constexpr bool SCREEN_FRAME_INTERVAL = true;
#else
static constexpr bool SCREEN_FRAME_INTERVAL = false;
#endif

#if (defined CONFIG_PACKAGE_X11 && !defined CONFIG_MACHINE_PANDORA) || defined CONFIG_BASE_MULTI_SCREEN
#define CONFIG_BASE_MULTI_WINDOW
static constexpr bool BASE_MULTI_WINDOW = true;
#else
static constexpr bool BASE_MULTI_WINDOW = false;
#endif

#if defined CONFIG_OS_IOS && defined __ARM_ARCH_6K__
#define CONFIG_GFX_SOFT_ORIENTATION 1
#elif !defined __ANDROID__ && !defined CONFIG_OS_IOS
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

constexpr bool threadPerformanceHints = Config::envIsAndroid;

constexpr bool multipleScreenFrameRates = Config::envIsAndroid;

constexpr bool cpuAffinity = Config::envIsAndroid || Config::envIsLinux;

constexpr bool freeformWindows = Config::envIsLinux;

constexpr bool windowFocus = Config::envIsAndroid || Config::envIsLinux;
}

namespace IG::Input
{

class Event;
class Device;
class KeyEvent;
class MotionEvent;

enum class DeviceChange: uint8_t { added, removed, updated, shown, hidden, connectError };

// Sent when a known input device addition/removal/change occurs
struct DeviceChangeEvent{const Device &device; DeviceChange change;};

// Sent when the device list is rebuilt, all devices should be re-checked
struct DevicesEnumeratedEvent{};

}

namespace IG
{

using OnFrameDelegate = DelegateFunc<bool (FrameParams params)>;

struct Orientations
{
	uint8_t
	portrait:1{},
	landscapeRight:1{},
	portraitUpsideDown:1{},
	landscapeLeft:1{};

	// TODO: use constexpr bit_cast with bit-fields when Clang supports it
	constexpr operator uint8_t() const { return portrait | landscapeRight << 1 | portraitUpsideDown << 2 | landscapeLeft << 3; }
	constexpr bool operator ==(Orientations const&) const = default;
	static constexpr Orientations allLandscape() { return {.landscapeRight = 1, .landscapeLeft = 1}; }
	static constexpr Orientations allPortrait() { return {.portrait = 1, .portraitUpsideDown = 1}; }
	static constexpr Orientations allButUpsideDown() { return {.portrait = 1, .landscapeRight = 1, .landscapeLeft = 1}; }
	static constexpr Orientations all() { return {.portrait = 1, .landscapeRight = 1, .portraitUpsideDown = 1, .landscapeLeft = 1}; }
};

std::string_view asString(Orientations);

WISE_ENUM_CLASS((Rotation, uint8_t),
	UP,
	RIGHT,
	DOWN,
	LEFT,
	ANY);

template<>
constexpr bool isValidProperty(const Rotation &v) { return enumIsValidUpToLast(v); }

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

struct WindowSurfaceChangeFlags
{
	using BitSetClassInt = uint8_t;

	BitSetClassInt
	surfaceResized:1{},
	contentRectResized:1{};
};

struct WindowSurfaceChange
{
	enum class Action : uint8_t
	{
		CREATED, CHANGED, DESTROYED
	};

	Action action{};
	WindowSurfaceChangeFlags flags{};

	constexpr WindowSurfaceChange(Action action, WindowSurfaceChangeFlags flags = {}):
		action{action}, flags{flags} {}
	constexpr bool resized() const { return action == Action::CHANGED; }
};

struct WindowDrawParams
{
	bool wasResized{};
	bool needsSync{};
};

enum class ScreenChange : int8_t { added, removed, frameRate };

WISE_ENUM_CLASS((SensorType, uint8_t),
	(Accelerometer, 1),
	(Gyroscope, 4),
	(Light, 5)
);

using SensorValues = std::array<float, 3>;

class Screen;
class Window;
struct WindowConfig;
class ApplicationContext;
class Application;
struct ApplicationInitParams;
class FrameTimer;
struct FDEventSourceDesc;
class FDEventSource;
class EventLoop;
struct TimerDesc;

using WindowContainer = std::vector<std::unique_ptr<Window>>;
using ScreenContainer = std::vector<std::unique_ptr<Screen>>;
using InputDeviceContainer = std::vector<std::unique_ptr<Input::Device>>;

// App events & delegates

// Sent when another process or the system file picker requests opening a document
struct DocumentPickerEvent{CStringView uri, displayName;};

// Sent when OS needs app to free any cached data
struct FreeCachesEvent{bool running;};

// Sent when a Screen is connected/disconnected or its properties change
struct ScreenChangeEvent{Screen &screen; ScreenChange change;};

using ApplicationEventVariant = std::variant<DocumentPickerEvent, FreeCachesEvent,
	ScreenChangeEvent, Input::DeviceChangeEvent, Input::DevicesEnumeratedEvent>;

class ApplicationEvent: public ApplicationEventVariant, public AddVisit
{
public:
	using ApplicationEventVariant::ApplicationEventVariant;
	using AddVisit::visit;
};

using OnApplicationEvent = DelegateFunc<void(ApplicationContext, const ApplicationEvent&)>;
using MainThreadMessageDelegate = DelegateFunc<void(ApplicationContext)>;
using ResumeDelegate = DelegateFunc<bool (ApplicationContext, bool focused)>;
using ExitDelegate = DelegateFunc<bool (ApplicationContext, bool backgrounded)>;
using DeviceOrientationChangedDelegate = DelegateFunc<void (ApplicationContext, Rotation newRotation)>;
using SystemOrientationChangedDelegate = DelegateFunc<void (ApplicationContext, Rotation oldRotation, Rotation newRotation)>;
using TextFieldDelegate = DelegateFunc<void (const char *str)>;
using SensorChangedDelegate = DelegateFunc<void (SensorValues)>;

// Window events & delegates

// Sent when the state of the window's drawing surface changes,
// such as a re-size or if it becomes the current drawing target
struct WindowSurfaceChangeEvent{WindowSurfaceChange change;};

// Sent during a Screen frame callback if the window needs to be drawn
struct DrawEvent{WindowDrawParams params;};

// Sent when app window enters/exits focus
struct FocusChangeEvent{bool in;};

// Sent when a file is dropped into into the app's window
// if app enables setAcceptDnd()
struct DragDropEvent{CStringView filename;};

// Sent when the user performs an action indicating to
// to the window manager they wish to dismiss the window
// (clicking the close button for example),
// by default it will exit the app
struct DismissRequestEvent{};

// Sent when the window is dismissed
struct DismissEvent{};

using WindowEventVariant = std::variant<WindowSurfaceChangeEvent, DrawEvent,
	Input::Event, FocusChangeEvent, DragDropEvent,
	DismissRequestEvent, DismissEvent>;

class WindowEvent;

using OnWindowEvent = DelegateFunc<bool(Window&, const WindowEvent&)>;
using WindowInitDelegate = DelegateFunc<void (ApplicationContext, Window&)>;

using PollEventFlags = int;
using PollEventDelegate = DelegateFunc<bool (int fd, PollEventFlags)>;

class CallbackDelegate : public DelegateFunc<bool ()>
{
public:
	using DelegateFuncBase::DelegateFuncBase;

	constexpr CallbackDelegate(Callable<void> auto&& f):
		DelegateFuncBase
		{
			[=]()
			{
				f();
				return false;
			}
		} {}
};

}
