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
#include <util/thread/pthread.hh>
#include <util/bits.h>
#include <util/rectangle2.h>
#include <util/Delegate.hh>

#if defined (CONFIG_BASE_X11) || (defined(CONFIG_BASE_ANDROID) && CONFIG_ENV_ANDROID_MINSDK < 9)
	#include <sys/epoll.h>
#elif defined(CONFIG_BASE_ANDROID) && CONFIG_ENV_ANDROID_MINSDK >= 9
	#include <android/looper.h>
#endif

#if defined(CONFIG_BASE_ANDROID)
	#include <base/android/public.hh>
#endif

namespace Base
{

struct Window : NotEquals<Window>
{
	constexpr Window() { }
	Rect2<int> rect; // active window content
	int w = 0, h = 0; // size of full window surface

	bool operator ==(Window const& rhs) const
	{
		return rect == rhs.rect && w == rhs.w && h == rhs.h;
	}
};

const Window &window();

// App exit
void exitVal(int returnVal) ATTRS(noreturn);
static void exit() { exitVal(0); }
void abort() ATTRS(noreturn);

// Console args
#if defined (CONFIG_BASE_X11) || defined (CONFIG_BASE_WIN32) \
	|| defined (CONFIG_BASE_SDL) || defined(CONFIG_BASE_GENERIC)
	uint numArgs();
	char * getArg(uint arg);
#else
	static uint numArgs() { return 0; }
	static char * getArg(uint arg) { return 0; }
#endif

// drag & drop
/*typedef void(*DragDropFunc)(void*, void*);
#if defined (CONFIG_BASE_X11) || defined (CONFIG_BASE_WIN32)
	uint getDragDropNumFiles(void *ddHnd);
	void getDragDropFilename(void *ddHnd, uint i, char* filename, uint bufferSize);
	void dragDropEventHandler(DragDropFunc handler, void *userPtr);
#else
	static uint getDragDropNumFiles(void *ddHnd) { return 0; };
	static void getDragDropFilename(void *ddHnd, uint i, char* filename, uint bufferSize) { }
	static void dragDropEventHandler(DragDropFunc handler, void *userPtr) { }
#endif*/

// app instance management
/*typedef void(*AltInstanceFunc)(void*, void*);
#if defined (CONFIG_BASE_X11) || defined (CONFIG_BASE_WIN32)
	void altInstanceEventHandler(AltInstanceFunc handler, void *userPtr);
#else
	static void altInstanceEventHandler(AltInstanceFunc handler, void *userPtr) { }
#endif*/

// Input device status

struct InputDevChange
{
	uint devId, devType;
	uint state;
	enum { ADDED, REMOVED, SHOWN, HIDDEN };

	bool added() const { return state == ADDED; }
	bool removed() const { return state == REMOVED; }
	bool shown() const { return state == SHOWN; }
	bool hidden() const { return state == HIDDEN; }
};

#if defined(CONFIG_ENV_WEBOS)
	static bool isInputDevPresent(uint type) { return 0; }
#else
	bool isInputDevPresent(uint type);
#endif

// Worker thread -> Main thread messages

static const ushort MSG_START = 127, MSG_INPUT = 127, MSG_INPUTDEV_CHANGE = 128, MSG_BT = 129,
		MSG_BT_SCAN_STATUS_DELEGATE = 130, MSG_ORIENTATION_CHANGE = 131,
		MSG_USER = 255;
void sendMessageToMain(ThreadPThread &thread, int type, int shortArg, int intArg, int intArg2);
// version used when thread context isn't needed
void sendMessageToMain(int type, int shortArg, int intArg, int intArg2);

/*static void sendInputMessageToMain(ThreadPThread &thread, uint dev, uint devType, uint btn, uint action)
{
	sendMessageToMain(thread, MSG_INPUT, dev, devType + (action << 16), btn);
}

static void sendInputDevChangeMessageToMain(ThreadPThread &thread, uint id, uint type, uint state)
{
	sendMessageToMain(thread, MSG_INPUTDEV_CHANGE, state, type, id);
}

static void sendBTMessageToMain(ThreadPThread &thread, uint event)
{
	sendMessageToMain(thread, MSG_BT, 0, event, 0);
}*/

static void sendBTScanStatusDelegate(ThreadPThread &thread, uint status, int arg = 0)
{
	sendMessageToMain(thread, MSG_BT_SCAN_STATUS_DELEGATE, 0, status, arg);
}

// Graphics update notification

void displayNeedsUpdate();

// App run state

static const uint APP_RUNNING = 0, APP_PAUSED = 1, APP_EXITING = 2;
extern uint appState;
static bool appIsRunning() { return appState == APP_RUNNING; }

// sleeping
void sleepMs(int ms); //sleep for ms milliseconds
void sleepUs(int us);

// window management
#if defined (CONFIG_BASE_X11) || defined (CONFIG_BASE_WIN32)
	void setWindowTitle(char *name);
#else
	static void setWindowTitle(char *name) { }
#endif

// DPI override
#if defined (CONFIG_BASE_X11) || defined (CONFIG_BASE_ANDROID)
	#define CONFIG_SUPPORTS_DPI_OVERRIDE
	void setDPI(float dpi);
#else
	static void setDPI(float dpi) { }
#endif

// display refresh rate
uint refreshRate();

// external services
#if defined (CONFIG_BASE_IOS)
	void openURL(const char *url);
#else
	static void openURL(const char *url) { }
#endif

// OpenGL windowing system support
#if defined (CONFIG_BASE_X11) || defined (CONFIG_BASE_WIN32) || defined (CONFIG_BASE_SDL)
	CallResult openGLInit();
	CallResult openGLSetOutputVideoMode(const Base::Window &win);
	CallResult openGLSetMultisampleVideoMode(const Base::Window &win);
#else
	static CallResult openGLInit() { return OK; }
	static CallResult openGLSetOutputVideoMode(const Base::Window &win) { return OK; }
	static CallResult openGLSetMultisampleVideoMode(const Base::Window &win) { return OK; }
#endif

// poll()-like event system support (WIP API)

typedef Delegate<int (int event)> PollEventDelegate;

#if defined(CONFIG_BASE_ANDROID) && CONFIG_ENV_ANDROID_MINSDK >= 9
	static const uint POLLEV_IN = ALOOPER_EVENT_INPUT, POLLEV_OUT = ALOOPER_EVENT_OUTPUT, POLLEV_ERR = ALOOPER_EVENT_ERROR;
#elif defined (CONFIG_BASE_X11) || (defined(CONFIG_BASE_ANDROID) && CONFIG_ENV_ANDROID_MINSDK < 9)
	static const uint POLLEV_IN = EPOLLIN, POLLEV_OUT = EPOLLOUT, POLLEV_ERR = EPOLLERR;
#endif

#if defined (CONFIG_BASE_X11) || defined(CONFIG_BASE_ANDROID)
	#define CONFIG_BASE_HAS_FD_EVENTS
	static const bool hasFDEvents = 1;
	void addPollEvent2(int fd, PollEventDelegate &handler, uint events = POLLEV_IN); // caller is in charge of handler's memory
	void modPollEvent(int fd, PollEventDelegate &handler, uint events);
	void removePollEvent(int fd); // unregister the fd (must still be open)
#else
	static const bool hasFDEvents = 0;
#endif

// timer event support
typedef void(*TimerCallbackFunc)(void*);
void setTimerCallback(TimerCallbackFunc f, void *ctx, int ms);

static void setTimerCallbackSec(TimerCallbackFunc f, void *ctx, int s)
{
	setTimerCallback(f, ctx, s * 1000);
}

// app file system paths
extern const char *appPath;
#if defined (CONFIG_BASE_ANDROID) || defined(CONFIG_BASE_IOS)
	const char *documentsPath();
#else
	static const char *documentsPath() { return appPath; }
#endif

#if defined (CONFIG_BASE_ANDROID) || defined(CONFIG_BASE_IOS) || defined(CONFIG_ENV_WEBOS)
	const char *storagePath();
#else
	static const char *storagePath() { return appPath; }
#endif

#if defined(CONFIG_BASE_IOS) && defined(CONFIG_BASE_IOS_JB)
	#define CONFIG_BASE_USES_SHARED_DOCUMENTS_DIR
#endif

// OS dialog & status bar management
#if defined CONFIG_BASE_IOS || defined CONFIG_BASE_ANDROID
	void setStatusBarHidden(uint hidden);
#else
	static void setStatusBarHidden(uint hidden) { }
#endif

#if defined CONFIG_BASE_IOS || defined CONFIG_ENV_WEBOS
	void setSystemOrientation(uint o);
#else
	static void setSystemOrientation(uint o) { }
#endif

static const uint OS_NAV_STYLE_DIM = BIT(0), OS_NAV_STYLE_HIDDEN = BIT(1);
#if defined(CONFIG_BASE_ANDROID)
	void setOSNavigationStyle(uint flags);
	bool hasHardwareNavButtons();
#else
	static void setOSNavigationStyle(uint flags) { }
	static bool hasHardwareNavButtons() { return 0; }
#endif

// orientation sensor support
#if defined(CONFIG_BASE_ANDROID) || defined(CONFIG_BASE_IOS) || CONFIG_ENV_WEBOS_OS >= 3
	void setAutoOrientation(bool on);
#else
	static void setAutoOrientation(bool on) { }
#endif

// vibration support
#if defined(CONFIG_BASE_ANDROID)
	void vibrate(uint ms);
#else
	static void vibrate(uint ms) { }
#endif

// UID functions
#ifdef CONFIG_BASE_IOS_SETUID
	extern uid_t realUID, effectiveUID;
	void setUIDReal();
	bool setUIDEffective();
#else
	static int realUID = 0, effectiveUID = 0;
	static void setUIDReal() { }
	static bool setUIDEffective() { return 0; }
#endif

// Device Identification
enum { DEV_TYPE_GENERIC,
#if defined (CONFIG_BASE_ANDROID)
	DEV_TYPE_XPERIA_PLAY, DEV_TYPE_XOOM,
#elif defined(CONFIG_BASE_IOS)
	DEV_TYPE_IPAD,
#endif
};

#if defined (CONFIG_BASE_ANDROID) || defined(CONFIG_BASE_IOS)
	int runningDeviceType();
#else
	static int runningDeviceType() { return DEV_TYPE_GENERIC; }
#endif

// Notification icon
#if defined (CONFIG_BASE_ANDROID)
	void addNotification(const char *onShow, const char *title, const char *message);
#else
	static void addNotification(const char *onShow, const char *title, const char *message) { }
#endif

// Power Saving
#if defined(CONFIG_BASE_ANDROID) || defined(CONFIG_BASE_IOS)
	void setIdleDisplayPowerSave(bool on);
#else
	static void setIdleDisplayPowerSave(bool on) { }
#endif

// App Callbacks

// Called when main receives a worker thread message with type >= MSG_USER
void onAppMessage(int type, int shortArg, int intArg, int intArg2);

// Called when app window enters/exits focus
void onFocusChange(uint in);

// Called when a known input device addition/removal/change occurs
void onInputDevChange(const InputDevChange &change);

// Called when app returns from backgrounded state
void onResume(bool focused);

// Called when app will finish execution
// If backgrounded == true, app may eventually resume execution
void onExit(bool backgrounded);

// Called on app startup
CallResult onInit() ATTRS(cold);

} // Base
