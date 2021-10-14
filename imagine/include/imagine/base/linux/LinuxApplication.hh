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
#ifdef CONFIG_BASE_DBUS
#include <gio/gio.h>
#endif
#include <imagine/base/EventLoop.hh>
#include <imagine/input/Device.hh>
#include <imagine/util/container/ArrayList.hh>
#include <vector>
#include <memory>

struct _XDisplay;
union _XEvent;

namespace Base
{
class LinuxApplication;
class ApplicationContext;
}

namespace Input
{

class EvdevInputDevice : public Device
{
public:
	EvdevInputDevice();
	EvdevInputDevice(int id, int fd, TypeBits, const char *name);
	~EvdevInputDevice();
	void processInputEvents(Base::LinuxApplication &app, input_event *event, uint32_t events);
	bool setupJoystickBits();
	void addPollEvent(Base::LinuxApplication &app);
	std::span<Axis> motionAxes() final;
	int fileDesc() const;

protected:
	static constexpr unsigned AXIS_SIZE = 24;
	int fd{-1};
	StaticArrayList<Axis, AXIS_SIZE> axis{};
	std::array<int, AXIS_SIZE> axisRangeOffset{};
	Base::FDEventSource fdSrc{-1};
};

}

namespace Base
{

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
	FS::PathString appPath() const;
	void setAppPath(FS::PathString);

protected:
	FDEventSource evdevSrc{};
	#ifdef CONFIG_BASE_DBUS
	GDBusConnection *gbus{};
	unsigned openPathSub{};
	uint32_t screenSaverCookie{};
	bool allowScreenSaver{true};
	#endif
	FS::PathString appPath_{};

	bool initDBus();
	void deinitDBus();
	void initEvdev(Base::EventLoop);
};

}
