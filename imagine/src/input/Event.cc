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
#include <imagine/base/Application.hh>

namespace Input
{

uint32_t Event::deviceID() const
{
	return devId;
}

const char *Event::mapName() const
{
	return mapName(map());
}

Map Event::map() const
{
	return map_;
}

void Event::setMap(Map map)
{
	map_ = map;
}

int Event::pointerID() const
{
	return pointerID_;
}

Action Event::state() const
{
	return state_;
}

void Event::setKeyFlags(uint8_t flags)
{
	keyFlags = flags;
}

bool Event::stateIsPointer() const
{
	return state() == Action::MOVED || state() == Action::EXIT_VIEW || state() == Action::ENTER_VIEW;
}

bool Event::isPointer() const
{
	return Config::Input::POINTING_DEVICES && (map() == Map::POINTER || stateIsPointer());
}

bool Event::isRelativePointer() const
{
	return Config::Input::RELATIVE_MOTION_DEVICES && state() == Action::MOVED_RELATIVE;
}

bool Event::isTouch() const
{
	return Config::Input::TOUCH_DEVICES && src == Source::TOUCHSCREEN;
}

bool Event::isKey() const
{
	return !isPointer() && !isRelativePointer();
}

bool Event::isGamepad() const
{
	return src == Source::GAMEPAD;
}

bool Event::isDefaultConfirmButton(uint32_t swapped) const
{
	switch(map())
	{
		default: return false;
		#ifdef CONFIG_BLUETOOTH
		case Map::WIIMOTE: return swapped ? isDefaultCancelButton(0) :
				(button == Input::Wiimote::_1 || button == Input::Wiimote::NUN_Z);
		case Map::WII_CC:
		case Map::ICONTROLPAD:
		case Map::ZEEMOTE:
			return swapped ? isDefaultCancelButton(0) : sysKey_ == Keycode::GAME_A;
		#endif
		#ifdef CONFIG_BLUETOOTH_SERVER
		case Map::PS3PAD: return swapped ? isDefaultCancelButton(0) : sysKey_ == Keycode::GAME_A;
		#endif
		case Map::ICADE: return swapped ? isDefaultCancelButton(0) : sysKey_ == Keycode::GAME_A;
		#ifdef CONFIG_INPUT_APPLE_GAME_CONTROLLER
		case Map::APPLE_GAME_CONTROLLER: return swapped ? isDefaultCancelButton(0) : sysKey_ == Keycode::GAME_A;
		#endif
		case Map::SYSTEM:
			switch(device()->subtype())
			{
				#ifdef CONFIG_MACHINE_PANDORA
				case Device::SUBTYPE_PANDORA_HANDHELD:
					return button == Input::Keycode::ENTER ||
						(swapped ? isDefaultCancelButton(0) : button == Keycode::Pandora::X);
				#endif
			}
			return button == Keycode::ENTER
			#ifdef CONFIG_BASE_ANDROID
			|| button == Keycode::CENTER
			#endif
			|| ((swapped && isGamepad()) ? isDefaultCancelButton(0) : (button == Keycode::GAME_A || button == Keycode::GAME_1));
	}
}

bool Event::isDefaultCancelButton(uint32_t swapped) const
{
	switch(map())
	{
		default: return false;
		#ifdef CONFIG_BLUETOOTH
		case Map::WIIMOTE: return swapped ? isDefaultConfirmButton(0) :
				(button == Input::Wiimote::_2 || button == Input::Wiimote::NUN_C);
		case Map::WII_CC:
		case Map::ICONTROLPAD:
		case Map::ZEEMOTE:
			return swapped ? isDefaultConfirmButton(0) : sysKey_ == Keycode::GAME_B;
		#endif
		#ifdef CONFIG_BLUETOOTH_SERVER
		case Map::PS3PAD: return swapped ? isDefaultConfirmButton(0) : sysKey_ == Keycode::GAME_B;
		#endif
		case Map::ICADE: return swapped ? isDefaultConfirmButton(0) : sysKey_ == Keycode::GAME_B;
		#ifdef CONFIG_INPUT_MOUSE_DEVICES
		case Map::POINTER: return button == Pointer::DOWN_BUTTON;
		#endif
		#ifdef CONFIG_INPUT_APPLE_GAME_CONTROLLER
		case Map::APPLE_GAME_CONTROLLER: return swapped ? isDefaultConfirmButton(0) : sysKey_ == Keycode::GAME_B;
		#endif
		case Map::SYSTEM:
			switch(device()->subtype())
			{
				#ifdef CONFIG_MACHINE_PANDORA
				case Device::SUBTYPE_PANDORA_HANDHELD:
					// TODO: can't call isDefaultConfirmButton(0) since it doesn't check whether the source was
					// a gamepad or keyboard
					return swapped ? (button == Keycode::Pandora::X) : (button == Keycode::Pandora::B);
				#endif
			}
			return button == Input::Keycode::ESCAPE || button == Input::Keycode::BACK
				|| ((swapped && isGamepad()) ? isDefaultConfirmButton(0) : (button == Input::Keycode::GAME_B || button == Input::Keycode::GAME_2));
	}
}

bool Event::isDefaultConfirmButton() const
{
	return isDefaultConfirmButton(hasSwappedConfirmKeys());
}

bool Event::isDefaultCancelButton() const
{
	return isDefaultCancelButton(hasSwappedConfirmKeys());
}

bool Event::isDefaultLeftButton() const
{
	switch(map())
	{
		default: return false;
		#ifdef CONFIG_BLUETOOTH
		case Map::WIIMOTE:
			return button == Input::Wiimote::LEFT || button == Input::Wiimote::NUN_STICK_LEFT;
		case Map::WII_CC:
			return button == Input::WiiCC::LEFT || button == Input::WiiCC::LSTICK_LEFT;
		case Map::ICONTROLPAD: return button == Input::iControlPad::LEFT || button == Input::iControlPad::LNUB_LEFT;
		case Map::ZEEMOTE: return button == Input::Zeemote::LEFT;
		#endif
		#ifdef CONFIG_BLUETOOTH_SERVER
		case Map::PS3PAD: return button == Input::PS3::LEFT || button == PS3::LSTICK_LEFT;
		#endif
		case Map::ICADE: return button == Input::ICade::LEFT;
		#ifdef CONFIG_INPUT_APPLE_GAME_CONTROLLER
		case Map::APPLE_GAME_CONTROLLER: return button == Input::AppleGC::LEFT;
		#endif
		case Map::SYSTEM:
			return button == Input::Keycode::LEFT
			|| button == Input::Keycode::JS1_XAXIS_NEG
			|| button == Input::Keycode::JS_POV_XAXIS_NEG
			;
	}
}

bool Event::isDefaultRightButton() const
{
	switch(map())
	{
		default: return false;
		#ifdef CONFIG_BLUETOOTH
		case Map::WIIMOTE:
			return button == Input::Wiimote::RIGHT || button == Input::Wiimote::NUN_STICK_RIGHT;
		case Map::WII_CC:
			return button == Input::WiiCC::RIGHT || button == Input::WiiCC::LSTICK_RIGHT;
		case Map::ICONTROLPAD: return button == Input::iControlPad::RIGHT || button == Input::iControlPad::LNUB_RIGHT;
		case Map::ZEEMOTE: return button == Input::Zeemote::RIGHT;
		#endif
		#ifdef CONFIG_BLUETOOTH_SERVER
		case Map::PS3PAD: return button == Input::PS3::RIGHT || button == PS3::LSTICK_RIGHT;
		#endif
		case Map::ICADE: return button == Input::ICade::RIGHT;
		#ifdef CONFIG_INPUT_APPLE_GAME_CONTROLLER
		case Map::APPLE_GAME_CONTROLLER: return button == Input::AppleGC::RIGHT;
		#endif
		case Map::SYSTEM:
			return button == Input::Keycode::RIGHT
			|| button == Input::Keycode::JS1_XAXIS_POS
			|| button == Input::Keycode::JS_POV_XAXIS_POS
			;
	}
}

bool Event::isDefaultUpButton() const
{
	switch(map())
	{
		default: return false;
		#ifdef CONFIG_BLUETOOTH
		case Map::WIIMOTE:
			return button == Input::Wiimote::UP || button == Input::Wiimote::NUN_STICK_UP;
		case Map::WII_CC:
			return button == Input::WiiCC::UP || button == Input::WiiCC::LSTICK_UP;
		case Map::ICONTROLPAD: return button == Input::iControlPad::UP || button == Input::iControlPad::LNUB_UP;
		case Map::ZEEMOTE: return button == Input::Zeemote::UP;
		#endif
		#ifdef CONFIG_BLUETOOTH_SERVER
		case Map::PS3PAD: return button == Input::PS3::UP || button == PS3::LSTICK_UP;
		#endif
		case Map::ICADE: return button == Input::ICade::UP;
		#ifdef CONFIG_INPUT_APPLE_GAME_CONTROLLER
		case Map::APPLE_GAME_CONTROLLER: return button == Input::AppleGC::UP;
		#endif
		case Map::SYSTEM:
			return button == Input::Keycode::UP
			|| button == Input::Keycode::JS1_YAXIS_NEG
			|| button == Input::Keycode::JS_POV_YAXIS_NEG
			;
	}
}

bool Event::isDefaultDownButton() const
{
	switch(map())
	{
		default: return false;
		#ifdef CONFIG_BLUETOOTH
		case Map::WIIMOTE:
			return button == Input::Wiimote::DOWN || button == Input::Wiimote::NUN_STICK_DOWN;
		case Map::WII_CC:
			return button == Input::WiiCC::DOWN || button == Input::WiiCC::LSTICK_DOWN;
		case Map::ICONTROLPAD: return button == Input::iControlPad::DOWN || button == Input::iControlPad::LNUB_DOWN;
		case Map::ZEEMOTE: return button == Input::Zeemote::DOWN;
		#endif
		#ifdef CONFIG_BLUETOOTH_SERVER
		case Map::PS3PAD: return button == Input::PS3::DOWN || button == PS3::LSTICK_DOWN;
		#endif
		case Map::ICADE: return button == Input::ICade::DOWN;
		#ifdef CONFIG_INPUT_APPLE_GAME_CONTROLLER
		case Map::APPLE_GAME_CONTROLLER: return button == Input::AppleGC::DOWN;
		#endif
		case Map::SYSTEM:
			return button == Input::Keycode::DOWN
			|| button == Input::Keycode::JS1_YAXIS_POS
			|| button == Input::Keycode::JS_POV_YAXIS_POS
			;
	}
}

bool Event::isDefaultDirectionButton() const
{
	return isDefaultLeftButton() || isDefaultRightButton() || isDefaultUpButton() || isDefaultDownButton();
}

bool Event::isDefaultPageUpButton() const
{
	switch(map())
	{
		default: return false;
		#ifdef CONFIG_BLUETOOTH
		case Map::WIIMOTE: return button == Input::Wiimote::PLUS;
		case Map::WII_CC: return button == Input::WiiCC::L;
		case Map::ICONTROLPAD: return button == Input::iControlPad::L;
		case Map::ZEEMOTE: return 0;
		#endif
		#ifdef CONFIG_BLUETOOTH_SERVER
		case Map::PS3PAD: return button == Input::PS3::L1;
		#endif
		case Map::ICADE: return button == Input::ICade::Z;
		#ifdef CONFIG_INPUT_APPLE_GAME_CONTROLLER
		case Map::APPLE_GAME_CONTROLLER: return button == Input::AppleGC::L1;
		#endif
		case Map::SYSTEM:
			switch(device()->subtype())
			{
				#ifdef CONFIG_MACHINE_PANDORA
				case Device::SUBTYPE_PANDORA_HANDHELD:
					return button == Keycode::Pandora::L;
				#endif
			}
			return button == Input::Keycode::PGUP
				|| button == Input::Keycode::GAME_L1;
	}
}

bool Event::isDefaultPageDownButton() const
{
	switch(map())
	{
		default: return false;
		#ifdef CONFIG_BLUETOOTH
		case Map::WIIMOTE: return button == Input::Wiimote::MINUS;
		case Map::WII_CC: return button == Input::WiiCC::R;
		case Map::ICONTROLPAD: return button == Input::iControlPad::R;
		case Map::ZEEMOTE: return 0;
		#endif
		#ifdef CONFIG_BLUETOOTH_SERVER
		case Map::PS3PAD: return button == Input::PS3::R1;
		#endif
		case Map::ICADE: return button == Input::ICade::C;
		#ifdef CONFIG_INPUT_APPLE_GAME_CONTROLLER
		case Map::APPLE_GAME_CONTROLLER: return button == Input::AppleGC::R1;
		#endif
		case Map::SYSTEM:
			switch(device()->subtype())
			{
				#ifdef CONFIG_MACHINE_PANDORA
				case Device::SUBTYPE_PANDORA_HANDHELD:
					return button == Keycode::Pandora::R;
				#endif
			}
			return button == Input::Keycode::PGDOWN
				|| button == Input::Keycode::GAME_R1;
	}
}

Key Event::key() const
{
	return sysKey_;
}

Key Event::mapKey() const
{
	return button;
}

#ifdef CONFIG_BASE_X11
void Event::setX11RawKey(Key key)
{
	rawKey = key;
}
#endif

bool Event::pushed() const
{
	return state() == Action::PUSHED;
}

bool Event::pushed(Key key) const
{
	return pushed() && button == key;
}

bool Event::pushedKey(Key sysKey) const
{
	return pushed() && sysKey_ == sysKey;
}

bool Event::released() const
{
	return state() == Action::RELEASED;
}

bool Event::released(Key key) const
{
	return released() && button == key;
}

bool Event::releasedKey(Key sysKey) const
{
	return released() && sysKey_ == sysKey;
}

bool Event::canceled() const
{
	return state() == Action::CANCELED;
}

bool Event::isOff() const
{
	return released() || canceled();
}

bool Event::moved() const
{
	return state() == Action::MOVED;
}

bool Event::isShiftPushed() const
{
	return metaState != 0;
}

int Event::repeated() const
{
	return repeatCount;
}

void Event::setRepeatCount(int count)
{
	repeatCount = count;
}

IG::WP Event::pos() const
{
	return {x, y};
}

bool Event::isPointerPushed(Key k) const
{
	if(released() && button == k)
		return false;
	if(pushed(k))
		return true;
	return metaState & IG::bit(k);
}

bool Event::isSystemFunction() const
{
	if(!isKey())
		return false;
	#ifdef __linux__
	switch(key())
	{
		default: return false;
		case Keycode::VOL_UP:
		case Keycode::VOL_DOWN:
			return true;
	}
	#else
	return false;
	#endif
}

const char *Event::actionToStr(Action action)
{
	switch(action)
	{
		default:
		case Action::UNUSED: return "Unknown";
		case Action::RELEASED: return "Released";
		case Action::PUSHED: return "Pushed";
		case Action::MOVED: return "Moved";
		case Action::MOVED_RELATIVE: return "Moved Relative";
		case Action::EXIT_VIEW: return "Left View";
		case Action::ENTER_VIEW: return "Entered View";
	}
}

Time Event::time() const
{
	return time_;
}

const Device *Event::device() const
{
	return device_;
}

bool Event::hasSwappedConfirmKeys() const
{
	return keyFlags; // currently there is only a single flag
}

}
