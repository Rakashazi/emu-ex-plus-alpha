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
#include <imagine/fs/sys.hh>
#include "dbus.hh"
#include "../common/basePrivate.hh"
#include "../x11/x11.hh"
#ifdef CONFIG_INPUT_EVDEV
#include "../../input/evdev/evdev.hh"
#endif

namespace Base
{

static FsSys::PathString appPath{};
extern void runMainEventLoop();
extern void initMainEventLoop();

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
	auto ret = system(makeFSPathStringPrintf("xdg-open %s", url).data());
}

const char *assetPath()
{
	return appPath.data();
}

const char *documentsPath()
{
	return appPath.data();
}

const char *storagePath()
{
	if(Config::MACHINE_IS_PANDORA)
	{
		static FsSys::PathString sdPath{};
		FsSys dir;
		// look for the first mounted SD card
		if(dir.openDir("/media", 0,
			[](const char *name, int type) -> int
			{
				return type == Fs::TYPE_DIR && strstr(name, "mmcblk");
			}
		) == OK)
		{
			if(dir.numEntries())
			{
				//logMsg("storage dir: %s", dir.entryFilename(0));
				string_printf(sdPath, "/media/%s", dir.entryFilename(0));
			}
			dir.closeDir();
			if(strlen(sdPath.data()))
			{
				return sdPath.data();
			}
		}
		// fall back to appPath
	}
	return appPath.data();
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

}

int main(int argc, char** argv)
{
	using namespace Base;
	doOrAbort(logger_init());
	engineInit();
	appPath = makeAppPathFromLaunchCommand(argv[0]);
	initMainEventLoop();
	#ifdef CONFIG_BASE_X11
	EventLoopFileSource x11Src;
	if(initWindowSystem(x11Src) != OK)
		return -1;
	#endif
	#ifdef CONFIG_INPUT_EVDEV
	Input::initEvdev();
	#endif
	doOrAbort(onInit(argc, argv));
	runMainEventLoop();
	return 0;
}
