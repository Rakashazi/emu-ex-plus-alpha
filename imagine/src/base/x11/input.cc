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
#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/Application.hh>
#include <imagine/base/Window.hh>
#include <imagine/input/Input.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/algorithm.h>
#include <imagine/util/string.h>
#include <imagine/util/ScopeGuard.hh>
#include "xlibutils.h"
#include <X11/XKBlib.h>
#include <X11/cursorfont.h>
#include <memory>

namespace Base
{

struct XIDeviceInfo : public ::XIDeviceInfo {};
struct XkbDescRec : public ::XkbDescRec {};

XInputDevice::XInputDevice() {}

XInputDevice::XInputDevice(uint32_t typeBits, const char *name):
	Device(0, Input::Map::SYSTEM, typeBits, name)
{}

XInputDevice::XInputDevice(XIDeviceInfo info, int enumId, bool isPointingDevice, bool isPowerButton):
	Device(enumId, Input::Map::SYSTEM, 0, info.name),
	id(info.deviceid)
{
	if(isPointingDevice)
	{
		type_ = Input::Device::TYPE_BIT_MOUSE;
	}
	else
	{
		type_ = Input::Device::TYPE_BIT_KEYBOARD;
		if(isPowerButton)
		{
			type_ |= Input::Device::TYPE_BIT_POWER_BUTTON;
		}
	}
}

void XInputDevice::setICadeMode(bool on)
{
	logMsg("set iCade mode %s for %s", on ? "on" : "off", name());
	iCadeMode_ = on;
}

bool XInputDevice::iCadeMode() const
{
	return iCadeMode_;
}

struct XIEventMaskData
{
	XIEventMask eventMask{};
	uint8_t maskBits[XIMaskLen(XI_LASTEVENT)]{};
};

const Input::Device *XApplication::deviceForInputId(int osId) const
{
	for(auto &dev : xDevice)
	{
		if(dev->id == osId)
		{
			return dev.get();
		}
	}
	if(!vkbDevice)
		logErr("device id %d doesn't exist", osId);
	return vkbDevice;
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

void XApplication::initPerWindowInputData(::Window win)
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

void XApplication::initXInput2()
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

int XApplication::devIdToPointer(int id) const
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

void XApplication::addXInputDevice(XIDeviceInfo xDevInfo, bool notify, bool isPointingDevice)
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
	uint32_t devId = 0;
	for(auto &e : systemInputDevices())
	{
		if(e->map() != Input::Map::SYSTEM)
			continue;
		if(string_equal(e->name(), xDevInfo.name) && e->enumId() == devId)
			devId++;
	}
	auto &devPtr = xDevice.emplace_back(std::make_unique<XInputDevice>(xDevInfo, devId, isPointingDevice, isPowerButtonName(xDevInfo.name)));
	if(Config::MACHINE_IS_PANDORA && (string_equal(xDevInfo.name, "gpio-keys")
		|| string_equal(xDevInfo.name, "keypad")))
	{
		devPtr->subtype_ = Input::Device::SUBTYPE_PANDORA_HANDHELD;
	}
	addSystemInputDevice(*devPtr, notify);
}

void XApplication::removeXInputDevice(int xDeviceId)
{
	if(auto removedDev = IG::moveOutIf(xDevice, [&](std::unique_ptr<XInputDevice> &dev){ return dev->id == xDeviceId; });
		removedDev)
	{
		removeSystemInputDevice(*removedDev, true);
		return;
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

static Input::Key keysymToKey(KeySym k)
{
	// if the keysym fits in 2 bytes leave as is,
	// otherwise use only first 15-bits to match
	// definition in Keycode namespace
	return k <= 0xFFFF ? k : k & 0xEFFF;
}

void XApplication::initInputSystem()
{
	initXInput2();

	// request input device changes events
	{
		XIEventMask eventMask;
		uint8_t mask[XIMaskLen(XI_LASTEVENT)] {0};
		XISetMask(mask, XI_HierarchyChanged);
		eventMask.deviceid = XIAllDevices;
		eventMask.mask_len = sizeof(mask);
		eventMask.mask = mask;
		XISelectEvents(dpy, DefaultRootWindow(dpy), &eventMask, 1);
	}

	// setup device list
	static XInputDevice virt{Input::Device::TYPE_BIT_VIRTUAL | Input::Device::TYPE_BIT_KEYBOARD | Input::Device::TYPE_BIT_KEY_MISC, "Virtual"};
	addSystemInputDevice(virt);
	vkbDevice = &virt;
	int devices;
	::XIDeviceInfo *device = XIQueryDevice(dpy, XIAllDevices, &devices);
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
				logMsg("mapping X pointer %d (%s) as pointer %d", device[i].deviceid, device[i].name, numCursors);
				xPointerMapping[numCursors] = device[i].deviceid;
				numCursors++;
				XIDeviceInfo d{device[i]};
				addXInputDevice(d, false, true);
			}
			bcase XISlaveKeyboard:
			{
				XIDeviceInfo d{device[i]};
				addXInputDevice(d, false, false);
			}
		}
	}
	XIFreeDeviceInfo(device);

	coreKeyboardDesc = static_cast<XkbDescRec*>(XkbGetKeyboard(dpy, XkbAllComponentsMask, XkbUseCoreKbd));
}

void XApplication::deinitInputSystem()
{
	//logMsg("deinit input data");
	if(blankCursor)
		XFreeCursor(dpy, blankCursor);
	if(normalCursor)
		XFreeCursor(dpy, normalCursor);
	if(coreKeyboardDesc)
		XkbFreeKeyboard(coreKeyboardDesc, XkbAllComponentsMask, true);
}

static uint32_t makePointerButtonState(XIButtonState state)
{
	uint8_t byte1 = state.mask_len > 0 ? state.mask[0] : 0;
	uint8_t byte2 = state.mask_len > 1 ? state.mask[1] : 0;
	return byte1 | (byte2 << 8);
}

// returns true if event is XI2, false otherwise
bool XApplication::handleXI2GenericEvent(XEvent event)
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
	if(ievent.evtype == XI_HierarchyChanged)
	{
		//logMsg("input device hierarchy changed");
		auto &ev = *((XIHierarchyEvent*)cookie->data);
		iterateTimes(ev.num_info, i)
		{
			if(ev.info[i].flags & XISlaveAdded)
			{
				int devices;
				::XIDeviceInfo *device = XIQueryDevice(dpy, ev.info[i].deviceid, &devices);
				if(devices)
				{
					if(device->use == XISlaveKeyboard)
					{
						XIDeviceInfo d{*device};
						addXInputDevice(d, true, false);
					}
					XIFreeDeviceInfo(device);
				}
			}
			else if(ev.info[i].flags & XISlaveRemoved)
			{
				removeXInputDevice(ev.info[i].deviceid);
			}
		}
		return true;
	}
	// others events are for specific windows
	auto destWin = windowForXWindow(ievent.event);
	if(!destWin) [[unlikely]]
	{
		//logWarn("ignored event for unknown window");
		return true;
	}
	auto &win = *destWin;
	auto time = IG::Milliseconds(ievent.time); // X11 timestamps are in ms
	auto updatePointer =
		[this](Base::Window &win, uint32_t key, uint32_t btnState, int p, Input::Action action, int x, int y, Input::Time time, int sourceID)
		{
			auto dev = deviceForInputId(sourceID);
			auto pos = win.transformInputPos({x, y});
			win.dispatchInputEvent(Input::Event{(uint32_t)p, Input::Map::POINTER, (Input::Key)key, btnState, action, pos.x, pos.y, p, Input::Source::MOUSE, time, dev});
		};
	auto handleKeyEvent =
		[this](Base::Window &win, XIDeviceEvent &ievent, Input::Time time, bool pushed)
		{
			auto action = pushed ? Input::Action::PUSHED : Input::Action::RELEASED;
			if(pushed)
				cancelKeyRepeatTimer();
			auto dev = deviceForInputId(ievent.sourceid);
			KeySym k = XkbKeycodeToKeysym(dpy, ievent.detail, 0, 0);
			bool repeated = ievent.flags & XIKeyRepeat;
			//logMsg("KeySym %d, KeyCode %d, repeat: %d", (int)k, ievent.detail, repeated);
			if(pushed && k == XK_Return && (ievent.mods.effective & (Mod1Mask | Mod5Mask)) && !repeated)
			{
				win.toggleFullScreen();
			}
			else
			{
				if(!dev->iCadeMode()
					|| (dev->iCadeMode() && !processICadeKey(k, action, time, *dev, win)))
				{
					bool isShiftPushed = ievent.mods.effective & ShiftMask;
					auto key = keysymToKey(k);
					auto ev = Input::Event{dev->enumId(), Input::Map::SYSTEM, key, key, action, isShiftPushed, repeated, Input::Source::KEYBOARD, time, dev};
					ev.setX11RawKey(ievent.detail);
					dispatchKeyInputEvent(ev, win);
				}
			}
		};
	//logMsg("device %d, event %s", ievent.deviceid, xIEventTypeToStr(ievent.evtype));
	switch(ievent.evtype)
	{
		bcase XI_ButtonPress:
			updatePointer(win, ievent.detail, makePointerButtonState(ievent.buttons),
				devIdToPointer(ievent.deviceid), Input::Action::PUSHED, ievent.event_x, ievent.event_y, time, ievent.sourceid);
		bcase XI_ButtonRelease:
			updatePointer(win, ievent.detail, makePointerButtonState(ievent.buttons),
				devIdToPointer(ievent.deviceid), Input::Action::RELEASED, ievent.event_x, ievent.event_y, time, ievent.sourceid);
		bcase XI_Motion:
			updatePointer(win, 0, makePointerButtonState(ievent.buttons), devIdToPointer(ievent.deviceid),
				Input::Action::MOVED, ievent.event_x, ievent.event_y, time, ievent.sourceid);
		bcase XI_Enter:
			updatePointer(win, 0, 0, devIdToPointer(ievent.deviceid), Input::Action::ENTER_VIEW,
				ievent.event_x, ievent.event_y, time, ievent.sourceid);
		bcase XI_Leave:
			updatePointer(win, 0, 0, devIdToPointer(ievent.deviceid), Input::Action::EXIT_VIEW,
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

Input::Event::KeyString XApplication::inputKeyString(Input::Key rawKey, uint32_t modifiers) const
{
	Input::Event::KeyString str{};
	KeySym k;
	XkbTranslateKeyCode(coreKeyboardDesc, rawKey, modifiers, nullptr, &k);
	XkbTranslateKeySym(dpy, &k, 0, str.data(), sizeof(str), nullptr);
	return str;
}

void ApplicationContext::flushSystemInputEvents()
{
	application().runX11Events();
}

}

namespace Input
{

bool Device::anyTypeBitsPresent(Base::ApplicationContext, uint32_t typeBits)
{
	// TODO
	if(typeBits & TYPE_BIT_KEYBOARD)
	{
		return 1;
	}
	return 0;
}

Event::KeyString Event::keyString(Base::ApplicationContext ctx) const
{
	return ctx.application().inputKeyString(rawKey, metaState ? ShiftMask : 0);
}

}

