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
#include <input/Input.hh>
#include <logger/interface.h>
#include <util/bits.h>
#include <input/common/common.h>
#include <input/DragPointer.hh>
#include <util/collection/ArrayList.hh>

using namespace Base;

#ifdef CONFIG_INPUT_ICADE
#include <input/common/iCade.hh>
#endif

namespace Input
{

static DragPointer dragStateArr[Input::maxCursors];
static Cursor blankCursor = (Cursor)0;
static Cursor normalCursor = (Cursor)0;
uint numCursors = 0;
bool translateKeycodes = 0;
static int xI2opcode = 0;
static int xPointerMapping[Input::maxCursors];
static XkbDescPtr coreKeyboardDesc = nullptr;
static Device *vkbDevice = nullptr;

struct XInputDevice : public Device
{
	int id = -1;
	char nameStr[80] {0};
	bool iCadeMode_ = false;

	constexpr XInputDevice() {}

	constexpr XInputDevice(uint typeBits, const char *name):
		Device(0, Event::MAP_SYSTEM, typeBits, name)
	{}

	XInputDevice(const XIDeviceInfo &info, int enumId):
		Device(enumId, Event::MAP_SYSTEM, Device::TYPE_BIT_KEYBOARD, nameStr),
		id(info.deviceid)
	{
		string_copy(nameStr, info.name);
	}

	#ifdef CONFIG_INPUT_ICADE
	void setICadeMode(bool on) override
	{
		logMsg("set iCade mode %s for %s", on ? "on" : "off", name());
		iCadeMode_ = on;
	}

	bool iCadeMode() const override
	{
		return iCadeMode_;
	}
	#endif
};

static StaticArrayList<XInputDevice*, 16> xDevice;

static const Device *deviceForInputId(int osId)
{
	iterateTimes(xDevice.size(), i)
	{
		if(xDevice[i]->id == osId)
		{
			return xDevice[i];
		}
	}
	if(!vkbDevice)
		bug_exit("device id %d doesn't exist", osId);
	return vkbDevice;
}

DragPointer *dragState(int p)
{
	return &dragStateArr[p];
}

void setKeyRepeat(bool on)
{
	allowKeyRepeats = on;
	if(!on)
	{
		deinitKeyRepeatTimer();
	}
}

void initPerWindowData(::Window win)
{
	if(Config::MACHINE_IS_PANDORA)
	{
		XFixesHideCursor(dpy, win);
	}
	else
	{
		if(!blankCursor)
		{
			// make a blank cursor
			char data[1] = {0};
			auto blank = XCreateBitmapFromData(dpy, win, data, 1, 1);
			if(blank == None)
			{
				logErr("unable to create blank cursor");
			}
			XColor dummy;
			blankCursor = XCreatePixmapCursor(dpy, blank, blank, &dummy, &dummy, 0, 0);
			XFreePixmap(dpy, blank);
			normalCursor = XCreateFontCursor(dpy, XC_left_ptr);
		}
	}
}

void hideCursor(::Window win)
{
	if(Config::MACHINE_IS_PANDORA)
		XFixesHideCursor(dpy, win);
	else
		XDefineCursor(dpy, win, Input::blankCursor);
}

void showCursor(::Window win)
{
	if(Config::MACHINE_IS_PANDORA)
		XFixesShowCursor(dpy, win);
	else
		XDefineCursor(dpy, win, Input::normalCursor);
}

bool Device::anyTypeBitsPresent(uint typeBits)
{
	// TODO
	if(typeBits & TYPE_BIT_KEYBOARD)
	{
		return 1;
	}
	return 0;
}

void setTranslateKeyboardEventsByModifiers(bool on)
{
	if(on)
		logMsg("translating key codes by modifier keys");
	else
		logMsg("using direct key codes");
	translateKeycodes = on;
}

bool translateKeyboardEventsByModifiers()
{
	return translateKeycodes;
}

static CallResult setupXInput2()
{
	// XInput Extension available?
	int event, error;
	if(!XQueryExtension(dpy, "XInputExtension", &xI2opcode, &event, &error))
	{
		logWarn("X Input extension not available");
		return INVALID_PARAMETER;
	}

	// Which version of XI
	int major = 2, minor = 0;
	if(XIQueryVersion(dpy, &major, &minor) == BadRequest)
	{
		logWarn("XI2 not available. Server supports %d.%d", major, minor);
		return INVALID_PARAMETER;
	}

	return OK;
}

static int devIdToPointer(int id)
{
	iterateTimes(4, i)
	{
		if(id == xPointerMapping[i])
			return i;
	}
	logWarn("warning: device id not present in pointer mapping");
	return 0;
}

static void addXInputDevice(const XIDeviceInfo &xDevInfo, bool notify)
{
	if(string_equal(xDevInfo.name, "Power Button")
		|| (Config::MACHINE_IS_PANDORA && string_equal(xDevInfo.name, "power-button"))
		|| string_equal(xDevInfo.name, "Virtual core XTEST keyboard"))
	{
		logMsg("skipping X key input device %d (%s)", xDevInfo.deviceid, xDevInfo.name);
		return;
	}
	for(auto &e : xDevice)
	{
		if(xDevInfo.deviceid == e->id)
		{
			logMsg("X key input device %d (%s) is already present", xDevInfo.deviceid, xDevInfo.name);
			return;
		}
	}
	logMsg("adding X key input device %d (%s) to device list", xDevInfo.deviceid, xDevInfo.name);
	uint devId = 0;
	for(auto &e : devList)
	{
		if(e->map() != Event::MAP_SYSTEM)
			continue;
		if(string_equal(e->name(), xDevInfo.name) && e->enumId() == devId)
			devId++;
	}
	auto dev = new XInputDevice(xDevInfo, devId);
	xDevice.push_back(dev);
	addDevice(*dev);
	if(Config::MACHINE_IS_PANDORA && (string_equal(xDevInfo.name, "gpio-keys")
		|| string_equal(xDevInfo.name, "keypad")))
	{
		dev->subtype_ = Device::SUBTYPE_PANDORA_HANDHELD;
	}
	if(notify)
		onInputDevChange(*dev, { Device::Change::ADDED });
}

static void removeXInputDevice(int xDeviceId)
{
	forEachInContainer(xDevice, e)
	{
		auto dev = *e;
		if(dev->id == xDeviceId)
		{
			auto removedDev = *dev;
			removeDevice(*dev);
			delete dev;
			xDevice.erase(e);
			onInputDevChange(removedDev, { Device::Change::REMOVED });
			return;
		}
	}
	logErr("key input device %d not in list", xDeviceId);
}

static const char *xInputDeviceTypeToStr(int type)
{
	switch(type)
	{
		case XIMasterPointer: return "Master Pointer";
		case XISlavePointer: return "Slave Pointer";
		case XIMasterKeyboard: return "Master Keyboard";
		case XISlaveKeyboard: return "Slave Keyboard";
		case XIFloatingSlave: return "Floating Slave";
		default: return "Unknown";
	}
}

CallResult init()
{
	if(setupXInput2() != OK)
	{
		exit();
	}

	// request input device changes events
	{
		XIEventMask eventMask;
		uchar mask[2] = { 0 };
		XISetMask(mask, XI_HierarchyChanged);
		eventMask.deviceid = XIAllDevices;
		eventMask.mask_len = sizeof(mask);
		eventMask.mask = mask;
		XISelectEvents(dpy, DefaultRootWindow(dpy), &eventMask, 1);
	}

	// setup device list
	static XInputDevice virt{Device::TYPE_BIT_VIRTUAL | Device::TYPE_BIT_KEYBOARD | Device::TYPE_BIT_KEY_MISC, "Virtual"};
	addDevice(virt);
	vkbDevice = &virt;
	int devices;
	XIDeviceInfo *device = XIQueryDevice(dpy, XIAllDevices, &devices);
	iterateTimes(devices, i)
	{
		if(device[i].use == XIMasterPointer || device[i].use == XISlaveKeyboard)
		{
			/*logMsg("Device %s (id: %d) %s paired to id %d",
				device[i].name, device[i].deviceid, xInputDeviceTypeToStr(device[i].use), device[i].attachment);*/
		}
		switch(device[i].use)
		{
			bcase XIMasterPointer:
			{
				logMsg("mapping X pointer %d (%s) as pointer %d", device[i].deviceid, device[i].name, Input::numCursors);
				xPointerMapping[Input::numCursors] = device[i].deviceid;
				Input::numCursors++;
			}
			bcase XISlaveKeyboard:
			{
				addXInputDevice(device[i], false);
			}
		}
	}
	XIFreeDeviceInfo(device);

	coreKeyboardDesc = XkbGetKeyboard(dpy, XkbAllComponentsMask, XkbUseCoreKbd);
	return OK;
}

}

static void updatePointer(Base::Window &win, uint event, int p, uint action, int x, int y, Input::Time time)
{
	using namespace Input;
	auto &state = dragStateArr[p];
	auto pos = pointerPos(win, x /*- win.viewRect.x*/, y /*- win.viewRect.y*/);
	state.pointerEvent(event, action, pos);
	Base::onInputEvent(win, Event(p, Event::MAP_POINTER, event, action, pos.x, pos.y, false, time, nullptr));
}

static void handlePointerButton(Base::Window &win, uint button, int p, uint action, int x, int y, Input::Time time)
{
	updatePointer(win, button, p, action, x, y, time);
}

static void handlePointerMove(Base::Window &win, int x, int y, int p, Input::Time time)
{
	//Input::m[p].inWin = 1;
	updatePointer(win, 0, p, Input::MOVED, x, y, time);
}

static void handlePointerEnter(Base::Window &win, int p, int x, int y, Input::Time time)
{
	//Input::m[p].inWin = 1;
	updatePointer(win, 0, p, Input::ENTER_VIEW, x, y, time);
}

static void handlePointerLeave(Base::Window &win, int p, int x, int y, Input::Time time)
{
	//Input::m[p].inWin = 0;
	updatePointer(win, 0, p, Input::EXIT_VIEW, x, y, time);
}

static void handleKeyEv(Base::Window &win, KeySym k, uint action, bool isShiftPushed, Input::Time time, const Input::Device *dev)
{
	//logMsg("got keysym %d", (int)k);
	Input::cancelKeyRepeatTimer();
	Base::onInputEvent(win, Input::Event(dev->enumId(), Input::Event::MAP_SYSTEM, k & 0xFFFF, action, isShiftPushed, time, dev));
}
