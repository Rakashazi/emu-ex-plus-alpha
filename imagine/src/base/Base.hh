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

#include <engine-globals.h>
#include <util/thread/sys.hh>
#include <util/bits.h>
#include <util/rectangle2.h>
#include <util/DelegateFunc.hh>
#include <base/Window.hh>

#if defined (CONFIG_BASE_X11) || (defined(CONFIG_BASE_ANDROID) && CONFIG_ENV_ANDROID_MINSDK < 9)
#include <sys/epoll.h>
#elif defined(CONFIG_BASE_ANDROID) && CONFIG_ENV_ANDROID_MINSDK >= 9
#include <android/looper.h>
#endif

#if defined(CONFIG_BASE_ANDROID)
#include <base/android/public.hh>
#elif defined(CONFIG_BASE_IOS)
#include <base/iphone/public.hh>
#endif

namespace Base
{

// Windows
Window &mainWindow();

// App exit
void exitVal(int returnVal);
static void exit() { exitVal(0); }
void abort() ATTRS(noreturn);

// Worker thread -> Main thread messages
static const ushort
	MSG_PLATFORM_START = 0, // platform-specific Base module message codes
	MSG_IMAGINE_START = 127, // shared message codes
	MSG_BT_SCAN_STATUS_DELEGATE = MSG_IMAGINE_START,
	MSG_USER = 255; // client app message codes
void sendMessageToMain(ThreadSys &thread, int type, int shortArg, int intArg, int intArg2);
// version used when thread context isn't needed
void sendMessageToMain(int type, int shortArg, int intArg, int intArg2);

static void sendBTScanStatusDelegate(ThreadSys &thread, uint status, int arg = 0)
{
	sendMessageToMain(thread, MSG_BT_SCAN_STATUS_DELEGATE, 0, status, arg);
}

// Inter-process messages
#if defined (CONFIG_BASE_X11)
void registerInstance(const char *name, int argc, char** argv);
#else
static void registerInstance(const char *name, int argc, char** argv) {}
#endif

// Graphics update notification
#if defined (CONFIG_BASE_ANDROID) || defined (CONFIG_BASE_IOS)
bool supportsFrameTime();
#else
static bool supportsFrameTime() { return 0; }
#endif

// App run state
static const uint APP_RUNNING = 0, APP_PAUSED = 1, APP_EXITING = 2;
extern uint appState;
static bool appIsRunning() { return appState == APP_RUNNING; }

// sleeping
void sleepMs(int ms); //sleep for ms milliseconds
void sleepUs(int us);

// display refresh rate
uint refreshRate();
static const uint REFRESH_RATE_DEFAULT = 0;
#ifdef CONFIG_BASE_X11
void setRefreshRate(uint rate);
#else
static void setRefreshRate(uint rate) {}
#endif

// external services
#if defined (CONFIG_BASE_IOS)
void openURL(const char *url);
#else
static void openURL(const char *url) {}
#endif

// poll()-like event system support
using PollEventDelegate = DelegateFunc<int (int event)>;
#if defined(CONFIG_BASE_ANDROID) && CONFIG_ENV_ANDROID_MINSDK >= 9
static const int POLLEV_IN = ALOOPER_EVENT_INPUT, POLLEV_OUT = ALOOPER_EVENT_OUTPUT,
	POLLEV_ERR = ALOOPER_EVENT_ERROR, POLLEV_HUP = ALOOPER_EVENT_HANGUP;
#elif defined (CONFIG_BASE_X11) || (defined(CONFIG_BASE_ANDROID) && CONFIG_ENV_ANDROID_MINSDK < 9)
static const int POLLEV_IN = EPOLLIN, POLLEV_OUT = EPOLLOUT, POLLEV_ERR = EPOLLERR, POLLEV_HUP = EPOLLHUP;
#endif

#if defined (CONFIG_BASE_X11) || defined(CONFIG_BASE_ANDROID)
void addPollEvent(int fd, PollEventDelegate &handler, uint events = POLLEV_IN); // caller is in charge of handler's memory
void modPollEvent(int fd, PollEventDelegate &handler, uint events);
void removePollEvent(int fd); // unregister the fd (must still be open)
#endif

// timer event support
using CallbackDelegate = DelegateFunc<void ()>;
struct CallbackRef;
void cancelCallback(CallbackRef *ref);
CallbackRef *callbackAfterDelay(CallbackDelegate callback, int ms);

static CallbackRef *callbackAfterDelaySec(CallbackDelegate callback, int s)
{
	return callbackAfterDelay(callback, s * 1000);
}

// file system paths
extern const char *appPath;
#if defined (CONFIG_BASE_ANDROID) || defined(CONFIG_BASE_IOS)
const char *documentsPath();
#else
static const char *documentsPath() { return appPath; }
#endif

const char *storagePath();

// OS dialog & status bar management
#if defined CONFIG_BASE_IOS || defined CONFIG_BASE_ANDROID
void setStatusBarHidden(bool hidden);
#else
static void setStatusBarHidden(bool hidden) {}
#endif

#if defined CONFIG_BASE_IOS || defined CONFIG_ENV_WEBOS
void setSystemOrientation(uint o);
#else
static void setSystemOrientation(uint o) {}
#endif

static const uint OS_NAV_STYLE_DIM = bit(0), OS_NAV_STYLE_HIDDEN = bit(1);
#if defined(CONFIG_BASE_ANDROID)
void setOSNavigationStyle(uint flags);
bool hasHardwareNavButtons();
#else
static void setOSNavigationStyle(uint flags) {}
static bool hasHardwareNavButtons() { return false; }
#endif

// orientation sensor support
#if defined(CONFIG_BASE_ANDROID) || defined(CONFIG_BASE_IOS) || CONFIG_ENV_WEBOS_OS >= 3
void setAutoOrientation(bool on);
#else
static void setAutoOrientation(bool on) {}
#endif

// vibration support
#if defined CONFIG_BASE_ANDROID && !defined CONFIG_MACHINE_OUYA
bool hasVibrator();
void vibrate(uint ms);
#define CONFIG_BASE_SUPPORTS_VIBRATOR
#else
static bool hasVibrator() { return false; }
static void vibrate(uint ms) {}
#endif

// Notification icon
#if defined (CONFIG_BASE_ANDROID)
void addNotification(const char *onShow, const char *title, const char *message);
#else
static void addNotification(const char *onShow, const char *title, const char *message) {}
#endif

// Power Saving
#if defined(CONFIG_BASE_ANDROID) || defined(CONFIG_BASE_IOS)
void setIdleDisplayPowerSave(bool on);
void endIdleByUserActivity();
#else
static void setIdleDisplayPowerSave(bool on) {}
static void endIdleByUserActivity() {}
#endif

// App Callbacks

// Called when main receives a worker thread message with type >= MSG_USER
void onAppMessage(int type, int shortArg, int intArg, int intArg2);

// Called when app window enters/exits focus
void onFocusChange(Base::Window &win, uint in);

// Called when a file is dropped into into the app's window
// if app enables setAcceptDnd()
void onDragDrop(Base::Window &win, const char *filename);

// Called when another process sends the app a message
void onInterProcessMessage(const char *filename);

// Called when app returns from backgrounded state
void onResume(bool focused);

// Called when app will finish execution
// If backgrounded == true, app may eventually resume execution
void onExit(bool backgrounded);

// Called on app startup, before the graphics context is initialized
CallResult onInit(int argc, char** argv) ATTRS(cold);

// Called on app window creation, after the graphics context is initialized
CallResult onWindowInit(Base::Window &win) ATTRS(cold);

} // Base

namespace Config
{
static constexpr bool BASE_SUPPORTS_VIBRATOR =
	#ifdef CONFIG_BASE_SUPPORTS_VIBRATOR
	true;
	#else
	false;
	#endif

#if defined(CONFIG_BASE_IOS) && defined(CONFIG_BASE_IOS_JB)
#define CONFIG_BASE_USES_SHARED_DOCUMENTS_DIR
static constexpr bool BASE_USES_SHARED_DOCUMENTS_DIR = true;
#else
static constexpr bool BASE_USES_SHARED_DOCUMENTS_DIR = false;
#endif

}
