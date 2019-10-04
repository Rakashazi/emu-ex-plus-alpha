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

#define LOGTAG "Input"
#include <imagine/base/Base.hh>
#include <imagine/input/Input.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/bits.h>
#include <imagine/util/algorithm.h>
#include <imagine/util/string.h>
#include <imagine/util/ScopeGuard.hh>
#include "xlibutils.h"
#include "x11.hh"
#include "internal.hh"
#include "../../input/private.hh"

using namespace Base;

namespace Input
{

struct XIEventMaskData
{
	XIEventMask eventMask{};
	uchar maskBits[XIMaskLen(XI_LASTEVENT)]{};
};

struct XInputDevice : public Device
{
	int id = -1;
	bool iCadeMode_ = false;

	XInputDevice() {}

	XInputDevice(uint typeBits, const char *name):
		Device(0, Event::MAP_SYSTEM, typeBits, name)
	{}

	XInputDevice(const XIDeviceInfo &info, int enumId, bool isPointingDevice, bool isPowerButton):
		Device(enumId, Event::MAP_SYSTEM, 0, info.name),
		id(info.deviceid)
	{
		if(isPointingDevice)
		{
			type_ = Device::TYPE_BIT_MOUSE;
		}
		else
		{
			type_ = Device::TYPE_BIT_KEYBOARD;
			if(isPowerButton)
			{
				type_ |= Device::TYPE_BIT_POWER_BUTTON;
			}
		}
	}

	void setICadeMode(bool on) final
	{
		logMsg("set iCade mode %s for %s", on ? "on" : "off", name());
		iCadeMode_ = on;
	}

	bool iCadeMode() const final
	{
		return iCadeMode_;
	}
};

static std::vector<std::unique_ptr<XInputDevice>> xDevice;
static Cursor blankCursor{};
static Cursor normalCursor{};
static uint numCursors = 0;
static int xI2opcode = 0;
static int xPointerMapping[Config::Input::MAX_POINTERS]{};
static XkbDescPtr coreKeyboardDesc{};
static Device *vkbDevice{};

static const Device *deviceForInputId(int osId)
{
	iterateTimes(xDevice.size(), i)
	{
		if(xDevice[i]->id == osId)
		{
			return xDevice[i].get();
		}
	}
	if(!vkbDevice)
		logErr("device id %d doesn't exist", osId);
	return vkbDevice;
}

void setKeyRepeat(bool on)
{
	setAllowKeyRepeats(on);
}

static void setXIEventMaskData(XIEventMaskData &data)
{
	data.eventMask.deviceid = XIAllMasterDevices;
	data.eventMask.mask_len = sizeof(data.maskBits); // always in bytes
	data.eventMask.mask = data.maskBits;
	XISetMask(data.maskBits, XI_ButtonPress);
	XISetMask(data.maskBits, XI_ButtonRelease);
	XISetMask(data.maskBits, XI_Motion);
	XISetMask(data.maskBits, XI_FocusIn);
	XISetMask(data.maskBits, XI_Enter);
	XISetMask(data.maskBits, XI_FocusOut);
	XISetMask(data.maskBits, XI_Leave);
	XISetMask(data.maskBits, XI_KeyPress);
	XISetMask(data.maskBits, XI_KeyRelease);
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
			char data[1]{};
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
	XIEventMaskData xiMask;
	setXIEventMaskData(xiMask);
	XISelectEvents(dpy, win, &xiMask.eventMask, 1);
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

static void setupXInput2(Display *dpy)
{
	int event, error;
	if(!XQueryExtension(dpy, "XInputExtension", &xI2opcode, &event, &error))
	{
		logErr("XInput extension not available");
		::exit(-1);
	}
	int major = 2, minor = 0;
	if(XIQueryVersion(dpy, &major, &minor) == BadRequest)
	{
		logErr("required XInput 2.0 version not available, server supports %d.%d", major, minor);
		::exit(-1);
	}
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

static bool isPowerButtonName(const char *name)
{
	return strstr(name, "Power Button")
		|| (Config::MACHINE_IS_PANDORA && strstr(name, "power-button"));
}

static void addXInputDevice(const XIDeviceInfo &xDevInfo, bool notify, bool isPointingDevice)
{
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
	xDevice.emplace_back(std::make_unique<XInputDevice>(xDevInfo, devId, isPointingDevice, isPowerButtonName(xDevInfo.name)));
	auto dev = xDevice.back().get();
	addDevice(*dev);
	if(Config::MACHINE_IS_PANDORA && (string_equal(xDevInfo.name, "gpio-keys")
		|| string_equal(xDevInfo.name, "keypad")))
	{
		dev->subtype_ = Device::SUBTYPE_PANDORA_HANDHELD;
	}
	if(notify)
		onDeviceChange.callCopySafe(*dev, { Device::Change::ADDED });
}

static void removeXInputDevice(int xDeviceId)
{
	forEachInContainer(xDevice, e)
	{
		auto dev = e->get();
		if(dev->id == xDeviceId)
		{
			auto removedDev = *dev;
			removeDevice(*dev);
			xDevice.erase(e.it);
			onDeviceChange.callCopySafe(removedDev, { Device::Change::REMOVED });
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

static Key keysymToKey(KeySym k)
{
	// if the keysym fits in 2 bytes leave as is,
	// otherwise use only first 15-bits to match
	// definition in Keycode namespace
	return k <= 0xFFFF ? k : k & 0xEFFF;
}

void init(Display *dpy)
{
	setupXInput2(dpy);

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
				addXInputDevice(device[i], false, true);
			}
			bcase XISlaveKeyboard:
			{
				addXInputDevice(device[i], false, false);
			}
		}
	}
	XIFreeDeviceInfo(device);

	coreKeyboardDesc = XkbGetKeyboard(dpy, XkbAllComponentsMask, XkbUseCoreKbd);
}

void deinit()
{
	//logMsg("deinit input data");
	if(blankCursor)
		XFreeCursor(dpy, blankCursor);
	if(normalCursor)
		XFreeCursor(dpy, normalCursor);
	if(coreKeyboardDesc)
		XkbFreeClientMap(coreKeyboardDesc, 0, true);
}

static uint makePointerButtonState(XIButtonState state)
{
	uchar byte1 = state.mask_len > 0 ? state.mask[0] : 0;
	uchar byte2 = state.mask_len > 1 ? state.mask[1] : 0;
	return byte1 | (byte2 << 8);
}

static void updatePointer(Base::Window &win, uint key, uint btnState, int p, uint action, int x, int y, Input::Time time, int sourceID)
{
	auto dev = deviceForInputId(sourceID);
	auto pos = transformInputPos(win, {x, y});
	win.dispatchInputEvent(Event{(uint)p, Event::MAP_POINTER, (Key)key, btnState, action, pos.x, pos.y, p, false, time, dev});
}

bool handleXI2GenericEvent(XEvent &event)
{
	assert(event.type == GenericEvent);
	if(event.xcookie.extension != xI2opcode)
	{
		return false;
	}
	if(!XGetEventData(dpy, &event.xcookie))
	{
		logMsg("error in XGetEventData for XI2 event");
		return true;
	}
	auto freeEventData = IG::scopeGuard([&]() { XFreeEventData(dpy, &event.xcookie); });
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
						Input::addXInputDevice(*device, true, false);
					}
					XIFreeDeviceInfo(device);
				}
			}
			else if(ev.info[i].flags & XISlaveRemoved)
			{
				Input::removeXInputDevice(ev.info[i].deviceid);
			}
		}
		return true;
	}
	// others events are for specific windows
	auto destWin = windowForXWindow(ievent.event);
	if(unlikely(!destWin))
	{
		//logWarn("ignored event for unknown window");
		return true;
	}
	auto &win = *destWin;
	auto time = Time::makeWithMSecs(ievent.time); // X11 timestamps are in ms
	auto handleKeyEvent =
		[](Base::Window &win, XIDeviceEvent &ievent, Time time, bool pushed)
		{
			auto action = pushed ? PUSHED : RELEASED;
			if(pushed)
				cancelKeyRepeatTimer();
			auto dev = deviceForInputId(ievent.sourceid);
			KeySym k = XkbKeycodeToKeysym(dpy, ievent.detail, 0, 0);
			bool repeated = ievent.flags & XIKeyRepeat;
			//logMsg("KeySym %d, KeyCode %d, repeat: %d", (int)k, ievent.detail, repeated);
			if(pushed && k == XK_Return && (ievent.mods.effective & Mod1Mask) && !repeated)
			{
				toggleFullScreen(win.xWin);
			}
			else if(!pushed || (pushed && (allowKeyRepeats() || !repeated)))
			{
				if(!dev->iCadeMode()
					|| (dev->iCadeMode() && !processICadeKey(k, action, time, *dev, win)))
				{
					bool isShiftPushed = ievent.mods.effective & ShiftMask;
					auto key = keysymToKey(k);
					auto ev = Event{dev->enumId(), Event::MAP_SYSTEM, key, key, action, isShiftPushed, repeated, time, dev};
					ev.setX11RawKey(ievent.detail);
					win.dispatchInputEvent(ev);
				}
			}
		};
	//logMsg("device %d, event %s", ievent.deviceid, xIEventTypeToStr(ievent.evtype));
	switch(ievent.evtype)
	{
		bcase XI_ButtonPress:
			updatePointer(win, ievent.detail, makePointerButtonState(ievent.buttons),
				devIdToPointer(ievent.deviceid), PUSHED, ievent.event_x, ievent.event_y, time, ievent.sourceid);
		bcase XI_ButtonRelease:
			updatePointer(win, ievent.detail, makePointerButtonState(ievent.buttons),
				devIdToPointer(ievent.deviceid), RELEASED, ievent.event_x, ievent.event_y, time, ievent.sourceid);
		bcase XI_Motion:
			updatePointer(win, 0, makePointerButtonState(ievent.buttons), devIdToPointer(ievent.deviceid),
				MOVED, ievent.event_x, ievent.event_y, time, ievent.sourceid);
		bcase XI_Enter:
			updatePointer(win, 0, 0, devIdToPointer(ievent.deviceid), ENTER_VIEW,
				ievent.event_x, ievent.event_y, time, ievent.sourceid);
		bcase XI_Leave:
			updatePointer(win, 0, 0, devIdToPointer(ievent.deviceid), EXIT_VIEW,
				ievent.event_x, ievent.event_y, time, ievent.sourceid);
		bcase XI_FocusIn:
			win.dispatchFocusChange(true);
		bcase XI_FocusOut:
			win.dispatchFocusChange(false);
			deinitKeyRepeatTimer();
		bcase XI_KeyPress:
			handleKeyEvent(win, ievent, time, true);
		bcase XI_KeyRelease:
			handleKeyEvent(win, ievent, time, false);
	}
	return true;
}

Event::KeyString Event::keyString() const
{
	KeyString str{};
	KeySym k;
	uint mods = metaState ? ShiftMask : 0;
	XkbTranslateKeyCode(Input::coreKeyboardDesc, rawKey, mods, nullptr, &k);
	XkbTranslateKeySym(dpy, &k, 0, str.data(), sizeof(KeyString), nullptr);
	return str;
}

void showSoftInput() {}
void hideSoftInput() {}
bool softInputIsActive() { return false; }

void flushEvents()
{
	Base::x11FDHandler();
}

}

