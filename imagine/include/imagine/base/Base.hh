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
#include <imagine/util/bits.h>
#include <imagine/util/rectangle2.h>
#include <imagine/util/DelegateFunc.hh>
#include <imagine/base/baseDefs.hh>
#include <imagine/fs/FSDefs.hh>
#if defined __ANDROID__
#include <imagine/base/android/android.hh>
#elif defined __APPLE__ && TARGET_OS_IPHONE
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
void registerInstance(const char *appID, int argc, char** argv);
void setAcceptIPC(const char *appID, bool on);

// App run state
static const uint APP_RUNNING = 0, APP_PAUSED = 1, APP_EXITING = 2;
uint appActivityState();
static bool appIsRunning() { return appActivityState() == APP_RUNNING; }

// external services
void openURL(const char *url);

// file system paths
FS::PathString assetPath();
FS::PathString documentsPath();
FS::PathString storagePath();
FS::PathString libPath();
bool documentsPathIsShared();

// OS UI management (status & navigation bar)

static constexpr uint
  SYS_UI_STYLE_NO_FLAGS = 0,
  SYS_UI_STYLE_DIM_NAV = bit(0),
  SYS_UI_STYLE_HIDE_NAV = bit(1),
  SYS_UI_STYLE_HIDE_STATUS = bit(2);

void setSysUIStyle(uint flags);
bool hasTranslucentSysUI();
bool hasHardwareNavButtons();
void setSystemOrientation(uint o);
uint defaultSystemOrientations();

// Sensors
void setDeviceOrientationChangeSensor(bool on);

// vibration support
bool hasVibrator();
void vibrate(uint ms);

// Notification/Launcher icons
void addNotification(const char *onShow, const char *title, const char *message);
void addLauncherIcon(const char *name, const char *path);

// Power Management
void setIdleDisplayPowerSave(bool on);
void endIdleByUserActivity();

// Permissions
enum Permission
{
	WRITE_EXT_STORAGE
};

bool usesPermission(Permission p);
bool requestPermission(Permission p);

// App Callbacks

using InterProcessMessageDelegate = DelegateFunc<void (const char *filename)>;
using ResumeDelegate = DelegateFunc<void (bool focused)>;
using FreeCachesDelegate = DelegateFunc<void ()>;
using ExitDelegate = DelegateFunc<void (bool backgrounded)>;
using DeviceOrientationChangedDelegate = DelegateFunc<void (uint newOrientation)>;
using SystemOrientationChangedDelegate = DelegateFunc<void (uint oldOrientation, uint newOrientation)>;

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

// Called when device changes orientation and the sensor is enabled
void setOnDeviceOrientationChanged(DeviceOrientationChangedDelegate del);

// Called when system UI changes orientation
void setOnSystemOrientationChanged(SystemOrientationChangedDelegate del);

// Called on app startup
[[gnu::cold]] void onInit(int argc, char** argv);

} // Base

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

}
