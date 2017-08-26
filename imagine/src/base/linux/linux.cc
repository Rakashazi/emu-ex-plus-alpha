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

#define LOGTAG "Base"
#include <imagine/logger/logger.h>
#include <imagine/base/Base.hh>
#include <imagine/fs/FS.hh>
#include <imagine/util/ScopeGuard.hh>
#include <imagine/util/string.h>
#include "dbus.hh"
#include "../common/basePrivate.hh"
#include "../x11/x11.hh"
#ifdef CONFIG_INPUT_EVDEV
#include "../../input/evdev/evdev.hh"
#endif
#include <cstring>

namespace Base
{

static FS::PathString appPath{};

uint appActivityState() { return APP_RUNNING; }

static void cleanup()
{
	#ifdef CONFIG_BASE_DBUS
	deinitDBus();
	#endif
	#ifdef CONFIG_BASE_X11
	deinitWindowSystem();
	#endif
}

void exit(int returnVal)
{
	dispatchOnExit(false);
	cleanup();
	::exit(returnVal);
}

void abort() { ::abort(); }

void openURL(const char *url)
{
	logMsg("opening url:%s", url);
	auto ret = system(FS::makePathStringPrintf("xdg-open %s", url).data());
}

FS::PathString assetPath()
{
	return appPath;
}

FS::PathString documentsPath()
{
	return appPath;
}

FS::PathString storagePath()
{
	if(Config::MACHINE_IS_PANDORA)
	{
		// look for the first mounted SD card
		for(auto &entry : FS::directory_iterator{"/media"})
		{
			if(entry.type() == FS::file_type::directory && std::strstr(entry.name(), "mmcblk"))
			{
				//logMsg("storage dir: %s", entry.path().data());
				return entry.path();
			}
		}
		// fall back to appPath
	}
	return appPath;
}

FS::PathString libPath()
{
	return appPath;
}

bool documentsPathIsShared()
{
	// TODO
	return false;
}

void setDeviceOrientationChangeSensor(bool on) {}

void setOnDeviceOrientationChanged(DeviceOrientationChangedDelegate del) {}

void setSystemOrientation(uint o) {}

uint defaultSystemOrientations()
{
	return VIEW_ROTATE_ALL;
}

void setOnSystemOrientationChanged(SystemOrientationChangedDelegate del) {}

bool usesPermission(Permission p)
{
	return false;
}

bool requestPermission(Permission p)
{
	return false;
}

void setIdleDisplayPowerSave(bool on) {}

void endIdleByUserActivity() {}

#ifndef CONFIG_BASE_DBUS
void registerInstance(const char *appID, int argc, char** argv) {}

void setAcceptIPC(const char *appID, bool on) {}
#endif

void addNotification(const char *onShow, const char *title, const char *message) {}

void addLauncherIcon(const char *name, const char *path) {}

bool hasVibrator() { return false; }

void vibrate(uint ms) {}

void exitWithErrorMessageVPrintf(int exitVal, const char *format, va_list args)
{
	std::array<char, 512> msg{};
	auto result = vsnprintf(msg.data(), msg.size(), format, args);
	auto cmd = string_makePrintf<1024>("zenity --warning --title='Exited with error' --text='%s'", msg.data());
	auto cmdResult = system(cmd.data());
	exit(exitVal);
}

}

int main(int argc, char** argv)
{
	using namespace Base;
	logger_init();
	engineInit();
	appPath = FS::makeAppPathFromLaunchCommand(argv[0]);
	auto eventLoop = EventLoop::makeForThread();
	#ifdef CONFIG_BASE_X11
	FDEventSource x11Src;
	if(initWindowSystem(eventLoop, x11Src) != OK)
		return -1;
	#endif
	#ifdef CONFIG_INPUT_EVDEV
	Input::initEvdev(eventLoop);
	#endif
	onInit(argc, argv);
	eventLoop.run();
	return 0;
}
