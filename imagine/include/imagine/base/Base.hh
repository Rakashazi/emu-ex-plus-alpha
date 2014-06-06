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

#include <imagine/engine-globals.h>
#include <imagine/util/thread/sys.hh>
#include <imagine/util/bits.h>
#include <imagine/util/rectangle2.h>
#include <imagine/util/DelegateFunc.hh>

#if defined CONFIG_BASE_ANDROID
#include <imagine/base/android/public.hh>
#elif defined CONFIG_BASE_IOS
#include <imagine/base/iphone/public.hh>
#endif

namespace Base
{
using namespace IG;

// App exit
void exit(int returnVal);
static void exit() { exit(0); }
[[noreturn]] void abort();

// Inter-process messages
#if defined CONFIG_BASE_DBUS
void registerInstance(const char *appID, int argc, char** argv);
void setAcceptIPC(const char *appID, bool on);
#else
static void registerInstance(const char *appID, int argc, char** argv) {}
static void setAcceptIPC(const char *appID, bool on) {}
#endif

// App run state
static const uint APP_RUNNING = 0, APP_PAUSED = 1, APP_EXITING = 2;
uint appActivityState();
static bool appIsRunning() { return appActivityState() == APP_RUNNING; }

// sleeping
void sleepMs(int ms); //sleep for ms milliseconds
void sleepUs(int us);

// external services
#if defined (CONFIG_BASE_IOS)
void openURL(const char *url);
#else
static void openURL(const char *url) {}
#endif

// file system paths
extern const char *appPath;
#if defined (CONFIG_BASE_ANDROID) || defined(CONFIG_BASE_IOS)
const char *documentsPath();
#else
static const char *documentsPath() { return appPath; }
#endif

const char *storagePath();

// OS UI management (status & navigation bar)

static constexpr uint
  SYS_UI_STYLE_NO_FLAGS = 0,
  SYS_UI_STYLE_DIM_NAV = bit(0),
  SYS_UI_STYLE_HIDE_NAV = bit(1),
  SYS_UI_STYLE_HIDE_STATUS = bit(2);

#if defined CONFIG_BASE_IOS || defined CONFIG_BASE_ANDROID
void setSysUIStyle(uint flags);
bool hasHardwareNavButtons();
#else
static void setSysUIStyle(uint flags) {}
static bool hasHardwareNavButtons() { return false; }
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

// Notification/Launcher icons
#if defined CONFIG_BASE_ANDROID
void addNotification(const char *onShow, const char *title, const char *message);
void addLauncherIcon(const char *name, const char *path);
#else
static void addNotification(const char *onShow, const char *title, const char *message) {}
static void addLauncherIcon(const char *name, const char *path) {}
#endif

// Power Management
#if defined(CONFIG_BASE_ANDROID) || defined(CONFIG_BASE_IOS)
void setIdleDisplayPowerSave(bool on);
void endIdleByUserActivity();
#else
static void setIdleDisplayPowerSave(bool on) {}
static void endIdleByUserActivity() {}
#endif

// App Callbacks

using InterProcessMessageDelegate = DelegateFunc<void (const char *filename)>;
using ResumeDelegate = DelegateFunc<void (bool focused)>;
using FreeCachesDelegate = DelegateFunc<void ()>;
using ExitDelegate = DelegateFunc<void (bool backgrounded)>;

// Called when another process sends the app a message
void setOnInterProcessMessage(InterProcessMessageDelegate del);
void dispatchOnInterProcessMessage(const char *filename);
const InterProcessMessageDelegate &onInterProcessMessage();

// Called when app returns from backgrounded state
void setOnResume(ResumeDelegate del);
void dispatchOnResume(bool focused);
const ResumeDelegate &onResume();

// Called when OS needs app to free any cached data
void setOnFreeCaches(FreeCachesDelegate del);
void dispatchOnFreeCaches();
const FreeCachesDelegate &onFreeCaches();

// Called when app will finish execution
// If backgrounded == true, app may eventually resume execution
void setOnExit(ExitDelegate del);
void dispatchOnExit(bool backgrounded);
const ExitDelegate &onExit();

// Called on app startup
[[gnu::cold]] CallResult onInit(int argc, char** argv);

} // Base

namespace Config
{

#ifdef CONFIG_BASE_SUPPORTS_VIBRATOR
static constexpr bool BASE_SUPPORTS_VIBRATOR = true;
#else
static constexpr bool BASE_SUPPORTS_VIBRATOR = false;
#endif

#if defined(CONFIG_BASE_IOS) && defined(CONFIG_BASE_IOS_JB)
#define CONFIG_BASE_USES_SHARED_DOCUMENTS_DIR
static constexpr bool BASE_USES_SHARED_DOCUMENTS_DIR = true;
#else
static constexpr bool BASE_USES_SHARED_DOCUMENTS_DIR = false;
#endif

}
