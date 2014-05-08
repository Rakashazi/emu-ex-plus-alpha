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

#include <imagine/input/Input.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/bits.h>
#include <imagine/input/DragPointer.hh>
#include <imagine/util/container/ArrayList.hh>
#include "x11.hh"
#include "../../input/private.hh"

using namespace Base;

namespace Input
{

static DragPointer dragStateArr[Input::maxCursors];
static Cursor blankCursor = (Cursor)0;
static Cursor normalCursor = (Cursor)0;
uint numCursors = 0;
bool translateKeycodes = false;
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
	setAllowKeyRepeats(on);
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

static CallResult setupXInput2(Display *dpy)
{
	int event, error;
	if(!XQueryExtension(dpy, "XInputExtension", &xI2opcode, &event, &error))
	{
		logErr("XInput extension not available");
		return INVALID_PARAMETER;
	}

	int major = 2, minor = 0;
	if(XIQueryVersion(dpy, &major, &minor) == BadRequest)
	{
		logErr("required XInput 2.0 version not available, server supports %d.%d", major, minor);
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
	if(notify && onDeviceChange)
		onDeviceChange(*dev, { Device::Change::ADDED });
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
			if(onDeviceChange)
				onDeviceChange(removedDev, { Device::Change::REMOVED });
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
	if(setupXInput2(dpy) != OK)
	{
		exit();
	}

	// request input device changes events
	{
		XIEventMask eventMask;
		uchar mask[XIMaskLen(XI_LASTEVENT)] {0};
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

static void updatePointer(Base::Window &win, uint key, int p, uint action, int x, int y, Input::Time time)
{
	auto &state = dragStateArr[p];
	auto pos = transformInputPos(win, {x, y});
	state.pointerEvent(key, action, pos);
	win.onInputEvent(win, Event(p, Event::MAP_POINTER, key, action, pos.x, pos.y, false, time, nullptr));
}

bool handleXI2GenericEvent(XEvent &event)
{
	assert(event.type == GenericEvent);
	if(event.xcookie.extension != Input::xI2opcode)
	{
		return false;
	}
	if(!XGetEventData(dpy, &event.xcookie))
	{
		logMsg("error in XGetEventData for XI2 event");
		return true;
	}

	XGenericEventCookie *cookie = &event.xcookie;
	auto &ievent = *((XIDeviceEvent*)cookie->data);
	// XI_HierarchyChanged isn't window-specific
	if(unlikely(ievent.evtype == XI_HierarchyChanged))
	{
		//logMsg("input device hierarchy changed");
		auto &ev = *((XIHierarchyEvent*)cookie->data);
		iterateTimes(ev.num_info, i)
		{
			if(ev.info[i].flags & XISlaveAdded)
			{
				int devices;
				XIDeviceInfo *device = XIQueryDevice(dpy, ev.info[i].deviceid, &devices);
				if(devices)
				{
					if(device->use == XISlaveKeyboard)
					{
						Input::addXInputDevice(*device, true);
					}
					XIFreeDeviceInfo(device);
				}
			}
			else if(ev.info[i].flags & XISlaveRemoved)
			{
				Input::removeXInputDevice(ev.info[i].deviceid);
			}
		}
		XFreeEventData(dpy, &event.xcookie);
		return true;
	}
	// others events are for specific windows
	auto destWin = windowForXWindow(ievent.event);
	if(unlikely(!destWin))
	{
		//logWarn("ignored event for unknown window");
		XFreeEventData(dpy, &event.xcookie);
		return true;
	}
	auto &win = *destWin;
	//logMsg("device %d, event %s", ievent.deviceid, xIEventTypeToStr(ievent.evtype));
	switch(ievent.evtype)
	{
		bcase XI_ButtonPress:
			updatePointer(win, ievent.detail, devIdToPointer(ievent.deviceid), PUSHED, ievent.event_x, ievent.event_y, ievent.time);
		bcase XI_ButtonRelease:
			updatePointer(win, ievent.detail, devIdToPointer(ievent.deviceid), RELEASED, ievent.event_x, ievent.event_y, ievent.time);
		bcase XI_Motion:
			updatePointer(win, 0, devIdToPointer(ievent.deviceid), MOVED, ievent.event_x, ievent.event_y, ievent.time);
		bcase XI_Enter:
			updatePointer(win, 0, devIdToPointer(ievent.deviceid), ENTER_VIEW, ievent.event_x, ievent.event_y, ievent.time);
		bcase XI_Leave:
			updatePointer(win, 0, devIdToPointer(ievent.deviceid), EXIT_VIEW, ievent.event_x, ievent.event_y, ievent.time);
		bcase XI_FocusIn:
			win.onFocusChange(win, 1);
		bcase XI_FocusOut:
			win.onFocusChange(win, 0);
		bcase XI_KeyPress:
		{
			Input::cancelKeyRepeatTimer();
			auto dev = Input::deviceForInputId(ievent.sourceid);
			KeySym k;
			if(Input::translateKeycodes)
			{
				unsigned int modsReturn;
				XkbTranslateKeyCode(Input::coreKeyboardDesc, ievent.detail, ievent.mods.effective, &modsReturn, &k);
			}
			else
				k = XkbKeycodeToKeysym(dpy, ievent.detail, 0, 0);
			bool repeated = ievent.flags & XIKeyRepeat;
			//logMsg("press KeySym %d, KeyCode %d, repeat: %d", (int)k, ievent.detail, repeated);
			if(k == XK_Return && (ievent.mods.effective & Mod1Mask) && !repeated)
			{
				toggleFullScreen(win.xWin);
			}
			else
			{
				using namespace Input;
				if(!repeated || Input::allowKeyRepeats())
				{
					//logMsg("push KeySym %d, KeyCode %d", (int)k, ievent.detail);
					#ifdef CONFIG_INPUT_ICADE
					if(!dev->iCadeMode()
						|| (dev->iCadeMode() && !processICadeKey(Keycode::decodeAscii(k, 0), Input::PUSHED, *dev, win)))
					#endif
					{
						bool isShiftPushed = ievent.mods.effective & ShiftMask;
						win.onInputEvent(win, Event(dev->enumId(), Event::MAP_SYSTEM, k & 0xFFFF, PUSHED, isShiftPushed, ievent.time, dev));
					}
				}
			}
		}
		bcase XI_KeyRelease:
		{
			auto dev = Input::deviceForInputId(ievent.sourceid);
			KeySym k;
			if(Input::translateKeycodes)
			{
				unsigned int modsReturn;
				XkbTranslateKeyCode(Input::coreKeyboardDesc, ievent.detail, ievent.mods.effective, &modsReturn, &k);
			}
			else
				k = XkbKeycodeToKeysym(dpy, ievent.detail, 0, 0);
			using namespace Input;
			//logMsg("release KeySym %d, KeyCode %d", (int)k, ievent.detail);
			#ifdef CONFIG_INPUT_ICADE
			if(!dev->iCadeMode()
				|| (dev->iCadeMode() && !processICadeKey(Keycode::decodeAscii(k, 0), Input::RELEASED, *dev, win)))
			#endif
			{
				win.onInputEvent(win, Event(dev->enumId(), Event::MAP_SYSTEM, k & 0xFFFF, RELEASED, 0, ievent.time, dev));
			}
		}
	}
	XFreeEventData(dpy, &event.xcookie);
	return true;
}

}

