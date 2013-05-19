#pragma once
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

static PointerState m[Input::maxCursors];
static DragPointer dragStateArr[Input::maxCursors];
static Cursor blankCursor = (Cursor)0;
static Cursor normalCursor = (Cursor)0;
uint numCursors = 0;
bool translateKeycodes = 0;
static int xI2opcode = 0;
static int xPointerMapping[Input::maxCursors];
static XkbDescPtr coreKeyboardDesc = nullptr;
static Device *vkbDevice = nullptr;

struct XInputDevice
{
	constexpr XInputDevice() {}
	XInputDevice(const XIDeviceInfo &info): id(info.deviceid)
	{
		string_copy(name, info.name);
	}
	Device *dev = nullptr;
	int id = 0;
	char name[80] {0};
};

static StaticArrayList<XInputDevice, 16> xDevice;

static const Device *deviceForInputId(int osId)
{
	iterateTimes(xDevice.size(), i)
	{
		if(xDevice.at(i).id == osId)
		{
			return xDevice.at(i).dev;
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

static bool allowKeyRepeats = 1;
void setKeyRepeat(bool on)
{
	allowKeyRepeats = on;
}

static void initPerWindowData(X11Window win)
{
	if(Config::MACHINE_IS_PANDORA)
	{
		XFixesHideCursor(dpy, win);
	}
	else
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

void hideCursor()
{
	if(Config::MACHINE_IS_PANDORA)
		XFixesHideCursor(dpy, win);
	else
		XDefineCursor(dpy, win, Input::blankCursor);
}

void showCursor()
{
	if(Config::MACHINE_IS_PANDORA)
		XFixesShowCursor(dpy, win);
	else
		XDefineCursor(dpy, win, Input::normalCursor);
}

bool Device::anyTypeBitsPresent(uint typeBits)
{
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
	forEachInContainer(xDevice, e)
	{
		if(xDevInfo.deviceid == e->id)
		{
			logMsg("X key input device %d (%s) is already present", xDevInfo.deviceid, xDevInfo.name);
			return;
		}
	}
	logMsg("adding X key input device %d (%s) to device list", xDevInfo.deviceid, xDevInfo.name);
	uint devId = 0;
	forEachInDLList(&devList, e)
	{
		if(e.map() != Event::MAP_KEYBOARD)
			continue;
		if(string_equal(e.name(), xDevInfo.name) && e.devId == devId)
			devId++;
	}
	xDevice.emplace_back(xDevInfo);
	addDevice(Device{devId, Event::MAP_KEYBOARD, Device::TYPE_BIT_KEYBOARD, xDevice.back().name});
	xDevice.back().dev = devList.last();
	if(Config::MACHINE_IS_PANDORA && (string_equal(xDevInfo.name, "gpio-keys")
		|| string_equal(xDevInfo.name, "keypad")))
	{
		xDevice.back().dev->subtype = Device::SUBTYPE_PANDORA_HANDHELD;
	}
	if(notify)
		onInputDevChange((DeviceChange){ 0, Event::MAP_KEYBOARD, DeviceChange::ADDED });
}

static void removeXInputDevice(int xDeviceId)
{
	forEachInContainer(xDevice, e)
	{
		if(e->id == xDeviceId)
		{
			removeDevice(*e->dev);
			xDevice.erase(e);
			onInputDevChange((DeviceChange){ 0, Event::MAP_KEYBOARD, DeviceChange::REMOVED });
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
	addDevice(Device{0, Event::MAP_KEYBOARD,
		Device::TYPE_BIT_VIRTUAL | Device::TYPE_BIT_KEYBOARD | Device::TYPE_BIT_KEY_MISC, "Virtual"});
	vkbDevice = devList.last();
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

static void updatePointer(uint event, int p, uint action, int x, int y)
{
	using namespace Input;
	auto &state = dragStateArr[p];
	auto pos = pointerPos(x, y);
	state.pointerEvent(event, action, pos);
	onInputEvent(Event(p, Event::MAP_POINTER, event, action, pos.x, pos.y, false, nullptr));
}

static void handlePointerButton(uint button, int p, uint action, int x, int y)
{
	updatePointer(button, p, action, x, y);
}

static void handlePointerMove(int x, int y, int p)
{
	Input::m[p].inWin = 1;
	updatePointer(0, p, Input::MOVED, x, y);
}

static void handlePointerEnter(int p, int x, int y)
{
	Input::m[p].inWin = 1;
	updatePointer(0, p, Input::ENTER_VIEW, x, y);
}

static void handlePointerLeave(int p, int x, int y)
{
	Input::m[p].inWin = 0;
	updatePointer(0, p, Input::EXIT_VIEW, x, y);
}

static void handleKeyEv(KeySym k, uint action, bool isShiftPushed, const Input::Device *dev)
{
	//logMsg("got keysym %d", (int)k);
	Input::onInputEvent(Input::Event(dev->devId, Input::Event::MAP_KEYBOARD, k & 0xFFFF, action, isShiftPushed, dev));
}
