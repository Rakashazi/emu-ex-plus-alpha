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
#include <imagine/input/Device.hh>
#include <imagine/base/Window.hh>
#include <imagine/base/Timer.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/Application.hh>
#include <imagine/util/algorithm.h>
#include <imagine/logger/logger.h>
#include <optional>

namespace IG::Input
{

static constexpr Key iCadeMap[12]
{
	Keycode::UP, Keycode::RIGHT, Keycode::DOWN, Keycode::LEFT,
	Keycode::GAME_X, Keycode::GAME_B,
	Keycode::GAME_A, Keycode::GAME_Y,
	Keycode::GAME_C, Keycode::GAME_Z,
	Keycode::GAME_START, Keycode::GAME_SELECT
};

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

std::string_view Event::mapName(Map map)
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

const char *sourceStr(Source src)
{
	switch(src)
	{
		case Source::UNKNOWN: break;
		case Source::KEYBOARD: return "Keyboard";
		case Source::GAMEPAD: return "Gamepad";
		case Source::MOUSE: return "Mouse";
		case Source::TOUCHSCREEN: return "Touchscreen";
		case Source::NAVIGATION: return "Navigation";
		case Source::JOYSTICK: return "Joystick";
	}
	return "Unknown";
}

const char *actionStr(Action act)
{
	switch(act)
	{
		case Action::UNUSED: return "Unused";
		case Action::RELEASED: return "Released";
		case Action::PUSHED: return "Pushed";
		case Action::MOVED: return "Moved";
		case Action::MOVED_RELATIVE: return "Moved Relative";
		case Action::EXIT_VIEW: return "Exit View";
		case Action::ENTER_VIEW: return "Enter View";
		case Action::SCROLL_UP: return "Scroll Up";
		case Action::SCROLL_DOWN: return "Scroll Down";
		case Action::CANCELED: return "Canceled";
	}
	return "Unknown";
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

DirectionKeys directionKeys(Map map)
{
	switch(map)
	{
		case Map::SYSTEM: return {Keycode::UP, Keycode::RIGHT, Keycode::DOWN, Keycode::LEFT};
		#ifdef CONFIG_BLUETOOTH
		case Map::WIIMOTE: return {Wiimote::UP, Wiimote::RIGHT, Wiimote::DOWN, Wiimote::LEFT};
		case Map::WII_CC: return {WiiCC::UP, WiiCC::RIGHT, WiiCC::DOWN, WiiCC::LEFT};
		case Map::ICONTROLPAD: return {iControlPad::UP, iControlPad::RIGHT, iControlPad::DOWN, iControlPad::LEFT};
		case Map::ZEEMOTE: return {Zeemote::UP, Zeemote::RIGHT, Zeemote::DOWN, Zeemote::LEFT};
		#endif
		#ifdef CONFIG_BLUETOOTH_SERVER
		case Map::PS3PAD: return {PS3::UP, PS3::RIGHT, PS3::DOWN, PS3::LEFT};
		#endif
		#ifdef CONFIG_INPUT_APPLE_GAME_CONTROLLER
		case Map::APPLE_GAME_CONTROLLER: return {AppleGC::UP, AppleGC::RIGHT, AppleGC::DOWN, AppleGC::LEFT};
		#endif
		default: return {};
	}
}

}

namespace IG
{

void BaseApplication::startKeyRepeatTimer(Input::Event event)
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
	if(!keyRepeatTimer) [[unlikely]]
	{
		keyRepeatTimer.emplace("keyRepeatTimer",
			[this]()
			{
				//logMsg("repeating key event");
				if(keyRepeatEvent.pushed()) [[likely]]
					dispatchKeyInputEvent(keyRepeatEvent);
				return true;
			});
	}
	keyRepeatTimer->run(IG::Milliseconds(400), IG::Milliseconds(50));
}

void BaseApplication::cancelKeyRepeatTimer()
{
	//logMsg("cancelled key repeat");
	if(!keyRepeatTimer)
		return;
	keyRepeatTimer->cancel();
	keyRepeatEvent = {};
}

void BaseApplication::deinitKeyRepeatTimer()
{
	keyRepeatTimer.reset();
}

void BaseApplication::setAllowKeyRepeatTimer(bool on)
{
	allowKeyRepeatTimer_ = on;
	if(!on)
	{
		deinitKeyRepeatTimer();
	}
}

const InputDeviceContainer &BaseApplication::inputDevices() const
{
	return inputDev;
}

Input::Device &BaseApplication::addInputDevice(std::unique_ptr<Input::Device> ptr, bool notify)
{
	ptr->setEnumId(nextInputDeviceEnumId(ptr->name()));
	auto &devPtr = inputDev.emplace_back(std::move(ptr));
	if(notify)
	{
		dispatchInputDeviceChange(*devPtr, {Input::DeviceAction::ADDED});
	}
	return *devPtr;
}

void BaseApplication::removeInputDevice(Input::Device &d, bool notify)
{
	removeInputDeviceIf([&](const auto &devPtr){ return devPtr.get() == &d; }, notify);
}

void BaseApplication::removeInputDevice(Input::Map map, int id, bool notify)
{
	removeInputDeviceIf([&](const auto &devPtr){ return devPtr->map() == map && devPtr->id() == id; }, notify);
}

void BaseApplication::removeInputDevices(Input::Map map, bool notify)
{
	while(auto removedDevice = IG::moveOutIf(inputDev, [&](const auto &iDev){ return iDev->map() == map; }))
	{
		if(notify)
		{
			dispatchInputDeviceChange(*removedDevice, {Input::DeviceAction::REMOVED});
		}
	}
}

void BaseApplication::removeInputDevice(InputDeviceContainer::iterator it, bool notify)
{
	if(it == inputDev.end()) [[unlikely]]
		return;
	auto removedDevPtr = std::move(*it);
	inputDev.erase(it);
	logMsg("removed input device:%s,%d", removedDevPtr->name().data(), removedDevPtr->enumId());
	cancelKeyRepeatTimer();
	if(notify)
	{
		dispatchInputDeviceChange(*removedDevPtr, {Input::DeviceAction::REMOVED});
	}
}

uint8_t BaseApplication::nextInputDeviceEnumId(std::string_view name) const
{
	static constexpr uint8_t maxEnum = 64;
	iterateTimes(maxEnum, i)
	{
		auto it = std::find_if(inputDev.begin(), inputDev.end(),
			[&](auto &devPtr){ return devPtr->name() == name && devPtr->enumId() == i; });
		if(it == inputDev.end())
			return i;
	}
	return maxEnum;
}

bool BaseApplication::dispatchRepeatableKeyInputEvent(Input::Event e, Window &win)
{
	e.setKeyFlags(swappedConfirmKeys());
	return win.dispatchRepeatableKeyInputEvent(e);
}

bool BaseApplication::dispatchRepeatableKeyInputEvent(Input::Event e)
{
	return dispatchRepeatableKeyInputEvent(e, mainWindow());
}

bool BaseApplication::dispatchKeyInputEvent(Input::Event e, Window &win)
{
	assert(e.isKey());
	e.setKeyFlags(swappedConfirmKeys());
	return win.dispatchInputEvent(e);
}

bool BaseApplication::dispatchKeyInputEvent(Input::Event e)
{
	return dispatchKeyInputEvent(e, mainWindow());
}

void BaseApplication::setOnInputDeviceChange(InputDeviceChangeDelegate del)
{
	onInputDeviceChange = del;
}

void BaseApplication::dispatchInputDeviceChange(const Input::Device &d, Input::DeviceChange change)
{
	onInputDeviceChange.callCopySafe(d, change);
}

void BaseApplication::setOnInputDevicesEnumerated(InputDevicesEnumeratedDelegate del)
{
	onInputDevicesEnumerated = del;
}

std::optional<bool> BaseApplication::swappedConfirmKeysOption() const
{
	if(swappedConfirmKeys() == Input::SWAPPED_CONFIRM_KEYS_DEFAULT)
		return {};
	return swappedConfirmKeys();
}

bool BaseApplication::swappedConfirmKeys() const
{
	return swappedConfirmKeys_;
}

void BaseApplication::setSwappedConfirmKeys(std::optional<bool> opt)
{
	if(!opt)
		return;
	swappedConfirmKeys_ = *opt;
}

uint8_t BaseApplication::keyEventFlags() const
{
	return swappedConfirmKeys();
}

bool ApplicationContext::keyInputIsPresent() const
{
	return Input::Device::anyTypeBitsPresent(*this, Input::Device::TYPE_BIT_KEYBOARD | Input::Device::TYPE_BIT_GAMEPAD);
}

void ApplicationContext::flushInputEvents()
{
	flushSystemInputEvents();
	flushInternalInputEvents();
}

void ApplicationContext::flushInternalInputEvents()
{
	// TODO
}

[[gnu::weak]] void ApplicationContext::flushSystemInputEvents() {}

bool BaseApplication::processICadeKey(Input::Key key, Input::Action action, Input::Time time, const Input::Device &dev, Window &win)
{
	using namespace IG::Input;
	if(auto onKey = keyToICadeOnKey(key))
	{
		if(action == Action::PUSHED)
		{
			//logMsg("pushed iCade keyboard key: %s", dev.keyName(key));
			dispatchRepeatableKeyInputEvent({Map::ICADE, onKey, onKey, Action::PUSHED, 0, 0, Source::GAMEPAD, time, &dev}, win);
		}
		return true;
	}
	if(auto offKey = keyToICadeOffKey(key))
	{
		if(action == Action::PUSHED)
		{
			dispatchRepeatableKeyInputEvent({Map::ICADE, offKey, offKey, Action::RELEASED, 0, 0, Source::GAMEPAD, time, &dev}, win);
		}
		return true;
	}
	return false; // not an iCade key
}

}
