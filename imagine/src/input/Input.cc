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
#include <imagine/base/Window.hh>
#include <imagine/base/Timer.hh>
#include <imagine/base/Base.hh>
#include <imagine/util/algorithm.h>
#include <imagine/logger/logger.h>
#include "private.hh"
#include <optional>

namespace Input
{

static std::optional<Base::Timer> keyRepeatTimer{};
static Event keyRepeatEvent{};
static bool allowKeyRepeatTimer_ = true;
DeviceChangeDelegate onDeviceChange{};
DevicesEnumeratedDelegate onDevicesEnumerated{};
std::vector<Device*> devList{};
bool swappedGamepadConfirm_ = SWAPPED_GAMEPAD_CONFIRM_DEFAULT;
static uint32_t xPointerTransform_ = POINTER_NORMAL;
static uint32_t yPointerTransform_ = POINTER_NORMAL;
static uint32_t pointerAxis_ = POINTER_NORMAL;

static constexpr Key iCadeMap[12]
{
	Keycode::UP, Keycode::RIGHT, Keycode::DOWN, Keycode::LEFT,
	Keycode::GAME_X, Keycode::GAME_B,
	Keycode::GAME_A, Keycode::GAME_Y,
	Keycode::GAME_C, Keycode::GAME_Z,
	Keycode::GAME_START, Keycode::GAME_SELECT
};

const std::vector<Device*> &deviceList()
{
	return devList;
}

static void setAllowKeyRepeatTimer(bool on)
{
	allowKeyRepeatTimer_ = on;
	if(!on)
	{
		deinitKeyRepeatTimer();
	}
}

static bool allowKeyRepeatTimer()
{
	return allowKeyRepeatTimer_;
}

void setHintKeyRepeat(bool on)
{
	setAllowKeyRepeatTimer(on);
}

void startKeyRepeatTimer(Event event)
{
	if(!allowKeyRepeatTimer_)
		return;
	if(!event.pushed())
	{
		// only repeat PUSHED action, otherwise cancel the timer
		//logMsg("repeat event is not for pushed action");
		cancelKeyRepeatTimer();
		return;
	}
	//logMsg("starting key repeat");
	keyRepeatEvent = event;
	keyRepeatEvent.setRepeatCount(1);
	if(unlikely(!keyRepeatTimer))
	{
		keyRepeatTimer.emplace("keyRepeatTimer",
			[]()
			{
				//logMsg("repeating key event");
				if(likely(keyRepeatEvent.pushed()))
					dispatchInputEvent(keyRepeatEvent);
				return true;
			});
	}
	keyRepeatTimer->run(IG::Milliseconds(400), IG::Milliseconds(50));
}

void cancelKeyRepeatTimer()
{
	//logMsg("cancelled key repeat");
	if(!keyRepeatTimer)
		return;
	keyRepeatTimer->cancel();
	keyRepeatEvent = {};
}

void deinitKeyRepeatTimer()
{
	keyRepeatTimer.reset();
}

void addDevice(Device &d)
{
	d.idx = devList.size();
	devList.emplace_back(&d);
}

void removeDevice(Device &d)
{
	logMsg("removing device: %s,%d", d.name(), d.enumId());
	cancelKeyRepeatTimer();
	IG::eraseFirst(devList, &d);
	indexDevices();
}

void indexDevices()
{
	// re-number device indices
	uint32_t i = 0;
	for(auto &e : Input::devList)
	{
		e->idx = i;
		i++;
	}
}

void xPointerTransform(uint32_t mode)
{
	xPointerTransform_ = mode;
}

void yPointerTransform(uint32_t mode)
{
	yPointerTransform_ = mode;
}

void pointerAxis(uint32_t mode)
{
	pointerAxis_ = mode;
}

IG::Point2D<int> transformInputPos(const Base::Window &win, IG::Point2D<int> srcPos)
{
	IG::Point2D<int> pos;
	// x,y axis is swapped first
	pos.x = pointerAxis_ == POINTER_INVERT ? srcPos.y : srcPos.x;
	pos.y = pointerAxis_ == POINTER_INVERT ? srcPos.x : srcPos.y;

	// then coordinates are inverted
	if(xPointerTransform_ == POINTER_INVERT)
		pos.x = win.width() - pos.x;
	if(yPointerTransform_ == POINTER_INVERT)
		pos.y = win.height() - pos.y;
	return pos;
}

// For soft-orientation
void configureInputForOrientation(const Base::Window &win)
{
	using namespace Input;
	using namespace Base;
	xPointerTransform(win.softOrientation() == VIEW_ROTATE_0 || win.softOrientation() == VIEW_ROTATE_90 ? POINTER_NORMAL : POINTER_INVERT);
	yPointerTransform(win.softOrientation() == VIEW_ROTATE_0 || win.softOrientation() == VIEW_ROTATE_270 ? POINTER_NORMAL : POINTER_INVERT);
	pointerAxis(win.softOrientation() == VIEW_ROTATE_0 || win.softOrientation() == VIEW_ROTATE_180 ? POINTER_NORMAL : POINTER_INVERT);
}

bool keyInputIsPresent()
{
	return Device::anyTypeBitsPresent(Device::TYPE_BIT_KEYBOARD | Device::TYPE_BIT_GAMEPAD);
}

bool dispatchInputEvent(Input::Event event)
{
	return Base::mainWindow().dispatchInputEvent(event);
}

void flushEvents()
{
	flushSystemEvents();
	flushInternalEvents();
}

void flushInternalEvents()
{
	// TODO
}

static Key keyToICadeOnKey(Key key)
{
	switch(key)
	{
		case Keycode::W : return iCadeMap[0];
		case Keycode::D : return iCadeMap[1];
		case Keycode::X : return iCadeMap[2];
		case Keycode::A : return iCadeMap[3];
		case Keycode::Y : return iCadeMap[4];
		case Keycode::H : return iCadeMap[5];
		case Keycode::U : return iCadeMap[6];
		case Keycode::J : return iCadeMap[7];
		case Keycode::I : return iCadeMap[8];
		case Keycode::K : return iCadeMap[9];
		case Keycode::O : return iCadeMap[10];
		case Keycode::L : return iCadeMap[11];
	}
	return 0;
}

static Key keyToICadeOffKey(Key key)
{
	switch(key)
	{
		case Keycode::E : return iCadeMap[0];
		case Keycode::C : return iCadeMap[1];
		case Keycode::Z : return iCadeMap[2];
		case Keycode::Q : return iCadeMap[3];
		case Keycode::T : return iCadeMap[4];
		case Keycode::R : return iCadeMap[5];
		case Keycode::F : return iCadeMap[6];
		case Keycode::N : return iCadeMap[7];
		case Keycode::M : return iCadeMap[8];
		case Keycode::P : return iCadeMap[9];
		case Keycode::G : return iCadeMap[10];
		case Keycode::V : return iCadeMap[11];
	}
	return 0;
}

bool processICadeKey(Key key, uint32_t action, Time time, const Device &dev, Base::Window &win)
{
	if(Key onKey = keyToICadeOnKey(key))
	{
		if(action == PUSHED)
		{
			//logMsg("pushed iCade keyboard key: %s", dev.keyName(key));
			Event event{0, Map::ICADE, onKey, onKey, PUSHED, 0, 0, Source::GAMEPAD, time, &dev};
			startKeyRepeatTimer(event);
			win.dispatchInputEvent(event);
		}
		return true;
	}
	if(Key offKey = keyToICadeOffKey(key))
	{
		if(action == PUSHED)
		{
			cancelKeyRepeatTimer();
			win.dispatchInputEvent({0, Map::ICADE, offKey, offKey, RELEASED, 0, 0, Source::GAMEPAD, time, &dev});
		}
		return true;
	}
	return false; // not an iCade key
}

void setOnDeviceChange(DeviceChangeDelegate del)
{
	onDeviceChange = del;
}

void setOnDevicesEnumerated(DevicesEnumeratedDelegate del)
{
	onDevicesEnumerated = del;
}

const char *Event::mapName(Map map)
{
	switch(map)
	{
		default: return "Unknown";
		case Map::SYSTEM: return "Key Input";
		case Map::POINTER: return "Pointer";
		case Map::REL_POINTER: return "Relative Pointer";
		#ifdef CONFIG_BLUETOOTH
		case Map::WIIMOTE: return "Wiimote";
		case Map::WII_CC: return "Classic / Wii U Pro Controller";
		case Map::ICONTROLPAD: return "iControlPad";
		case Map::ZEEMOTE: return "Zeemote JS1";
		#endif
		#ifdef CONFIG_BLUETOOTH_SERVER
		case Map::PS3PAD: return "PS3 Gamepad";
		#endif
		case Map::ICADE: return "iCade";
		#ifdef CONFIG_INPUT_APPLE_GAME_CONTROLLER
		case Map::APPLE_GAME_CONTROLLER: return "Apple Game Controller";
		#endif
	}
}

uint32_t Event::mapNumKeys(Map map)
{
	switch(map)
	{
		default: return 0;
		case Map::SYSTEM: return Input::Keycode::COUNT;
		#ifdef CONFIG_BLUETOOTH
		case Map::WIIMOTE: return Input::Wiimote::COUNT;
		case Map::WII_CC: return Input::WiiCC::COUNT;
		case Map::ICONTROLPAD: return Input::iControlPad::COUNT;
		case Map::ZEEMOTE: return Input::Zeemote::COUNT;
		#endif
		#ifdef CONFIG_BLUETOOTH_SERVER
		case Map::PS3PAD: return Input::PS3::COUNT;
		#endif
		case Map::ICADE: return Input::ICade::COUNT;
		#ifdef CONFIG_INPUT_APPLE_GAME_CONTROLLER
		case Map::APPLE_GAME_CONTROLLER: return Input::AppleGC::COUNT;
		#endif
	}
}

Event defaultEvent()
{
	Event e{};
	e.setMap(keyInputIsPresent() ? Map::SYSTEM : Map::POINTER);
	return e;
}

#ifndef CONFIG_INPUT_SYSTEM_COLLECTS_TEXT
uint32_t startSysTextInput(InputTextDelegate callback, const char *initialText, const char *promptText, uint32_t fontSizePixels)
{
	return 0;
}

void cancelSysTextInput() {}

void finishSysTextInput() {}

void placeSysTextInput(IG::WindowRect rect) {}

IG::WindowRect sysTextInputRect()
{
	return {};
}
#endif

void setSwappedGamepadConfirm(bool swapped)
{
	swappedGamepadConfirm_ = swapped;
}

bool swappedGamepadConfirm()
{
	return swappedGamepadConfirm_;
}

const char *sourceStr(Source src)
{
	switch(src)
	{
		default: return "Unknown";
		case Source::KEYBOARD: return "Keyboard";
		case Source::GAMEPAD: return "Gamepad";
		case Source::MOUSE: return "Mouse";
		case Source::TOUCHSCREEN: return "Touchscreen";
		case Source::NAVIGATION: return "Navigation";
		case Source::JOYSTICK: return "Joystick";
	}
}

Map validateMap(uint8_t mapValue)
{
	switch(mapValue)
	{
		default: return Map::UNKNOWN;
		case (uint8_t)Map::SYSTEM:
		#ifdef CONFIG_BLUETOOTH
		case (uint8_t)Map::WIIMOTE:
		case (uint8_t)Map::WII_CC:
		case (uint8_t)Map::ICONTROLPAD:
		case (uint8_t)Map::ZEEMOTE:
		#endif
		#ifdef CONFIG_BLUETOOTH_SERVER
		case (uint8_t)Map::PS3PAD:
		#endif
		case (uint8_t)Map::ICADE:
		#ifdef CONFIG_INPUT_APPLE_GAME_CONTROLLER
		case (uint8_t)Map::APPLE_GAME_CONTROLLER:
		#endif
		return Map(mapValue);
	}
}

}
