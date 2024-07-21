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
#include <imagine/base/BaseApplication.hh>
#include <imagine/fs/FSDefs.hh>
#if CONFIG_PACKAGE_DBUS
#include <gio/gio.h>
#endif
#include <imagine/base/EventLoop.hh>
#include <memory>

struct _XDisplay;
union _XEvent;

namespace IG
{

class LinuxApplication;

struct ApplicationInitParams
{
	EventLoop eventLoop;
	ApplicationContext *ctxPtr;
	int argc;
	char **argv;

	constexpr CommandArgs commandArgs() const
	{
		return {argc, argv};
	}
};

class LinuxApplication : public BaseApplication
{
public:
	LinuxApplication(ApplicationInitParams);
	~LinuxApplication();
	void setIdleDisplayPowerSave(bool wantsAllowScreenSaver);
	void endIdleByUserActivity();
	bool registerInstance(ApplicationInitParams, const char *name);
	void setAcceptIPC(bool on, const char *name);
	const FS::PathString &appPath() const;
	void setAppPath(FS::PathString);

protected:
	FDEventSource evdevSrc;
	#if CONFIG_PACKAGE_DBUS
	GDBusConnection *gbus{};
	unsigned openPathSub{};
	uint32_t screenSaverCookie{};
	bool allowScreenSaver{true};
	#endif
	FS::PathString appPath_{};

	bool initDBus();
	void deinitDBus();
	void initEvdev(EventLoop);
};

}
