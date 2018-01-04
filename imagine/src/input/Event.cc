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
#include "private.hh"

namespace Input
{

uint Event::deviceID() const
{
	return devId;
}

const char *Event::mapName()
{
	return mapName(map());
}

uint Event::map() const
{
	return map_;
}

void Event::setMap(uint map)
{
	map_ = map;
}

int Event::pointerID() const
{
	return pointerID_;
}

uint Event::state() const
{
	return state_;
}

bool Event::stateIsPointer() const
{
	return state() == MOVED || state() == EXIT_VIEW || state() == ENTER_VIEW;
}

bool Event::isPointer() const
{
	return Config::Input::POINTING_DEVICES && (map() == MAP_POINTER || stateIsPointer());
}

bool Event::isRelativePointer() const
{
	return Config::Input::RELATIVE_MOTION_DEVICES && state() == MOVED_RELATIVE;
}

bool Event::isTouch() const
{
	return Config::Input::TOUCH_DEVICES && pointerIsTouch;
}

bool Event::isKey() const
{
	return !isPointer() && !isRelativePointer();
}

bool Event::isDefaultConfirmButton(uint swapped) const
{
	switch(map())
	{
		#ifdef CONFIG_BLUETOOTH
		case MAP_WIIMOTE: return swapped ? isDefaultCancelButton(0) :
				(button == Input::Wiimote::_1 || button == Input::Wiimote::NUN_Z);
		case MAP_WII_CC:
		case MAP_ICONTROLPAD:
		case MAP_ZEEMOTE:
			return swapped ? isDefaultCancelButton(0) : sysKey_ == Keycode::GAME_A;
		#endif
		case MAP_ICADE: return swapped ? isDefaultCancelButton(0) : sysKey_ == Keycode::GAME_A;
		#if defined CONFIG_BASE_PS3 || defined CONFIG_BLUETOOTH
		case MAP_PS3PAD: return swapped ? isDefaultCancelButton(0) : sysKey_ == Keycode::GAME_A;
		#endif
		#ifdef CONFIG_INPUT_APPLE_GAME_CONTROLLER
		case MAP_APPLE_GAME_CONTROLLER: return swapped ? isDefaultCancelButton(0) : sysKey_ == Keycode::GAME_A;
		#endif
		case MAP_SYSTEM:
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
			|| (swapped ? isDefaultCancelButton(0) : (button == Keycode::GAME_A || button == Keycode::GAME_1));
	}
	return false;
}

bool Event::isDefaultConfirmButton() const
{
	return isDefaultConfirmButton(swappedGamepadConfirm_);
}

bool Event::isDefaultCancelButton(uint swapped) const
{
	switch(map())
	{
		#ifdef CONFIG_BLUETOOTH
		case MAP_WIIMOTE: return swapped ? isDefaultConfirmButton(0) :
				(button == Input::Wiimote::_2 || button == Input::Wiimote::NUN_C);
		case MAP_WII_CC:
		case MAP_ICONTROLPAD:
		case MAP_ZEEMOTE:
			return swapped ? isDefaultConfirmButton(0) : sysKey_ == Keycode::GAME_B;
		#endif
		case MAP_ICADE: return swapped ? isDefaultConfirmButton(0) : sysKey_ == Keycode::GAME_B;
		#if defined CONFIG_BASE_PS3 || defined CONFIG_BLUETOOTH
		case MAP_PS3PAD: return swapped ? isDefaultConfirmButton(0) : sysKey_ == Keycode::GAME_B;
		#endif
		#ifdef CONFIG_INPUT_MOUSE_DEVICES
		case MAP_POINTER: return button == Pointer::DOWN_BUTTON;
		#endif
		#ifdef CONFIG_INPUT_APPLE_GAME_CONTROLLER
		case MAP_APPLE_GAME_CONTROLLER: return swapped ? isDefaultConfirmButton(0) : sysKey_ == Keycode::GAME_B;
		#endif
		case MAP_SYSTEM:
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
				|| (swapped ? isDefaultConfirmButton(0) : (button == Input::Keycode::GAME_B || button == Input::Keycode::GAME_2));
	}
	return false;
}

bool Event::isDefaultCancelButton() const
{
	return isDefaultCancelButton(swappedGamepadConfirm_);
}

bool Event::isDefaultLeftButton() const
{
	switch(map())
	{
		#ifdef CONFIG_BLUETOOTH
		case MAP_WIIMOTE:
			return button == Input::Wiimote::LEFT || button == Input::Wiimote::NUN_STICK_LEFT;
		case MAP_WII_CC:
			return button == Input::WiiCC::LEFT || button == Input::WiiCC::LSTICK_LEFT;
		case MAP_ICONTROLPAD: return button == Input::iControlPad::LEFT || button == Input::iControlPad::LNUB_LEFT;
		case MAP_ZEEMOTE: return button == Input::Zeemote::LEFT;
		#endif
		case MAP_ICADE: return button == Input::ICade::LEFT;
		#if defined CONFIG_BASE_PS3 || defined CONFIG_BLUETOOTH
		case MAP_PS3PAD: return button == Input::PS3::LEFT || button == PS3::LSTICK_LEFT;
		#endif
		#ifdef CONFIG_INPUT_APPLE_GAME_CONTROLLER
		case MAP_APPLE_GAME_CONTROLLER: return button == Input::AppleGC::LEFT;
		#endif
		case MAP_SYSTEM:
			return button == Input::Keycode::LEFT
			|| button == Input::Keycode::JS1_XAXIS_NEG
			|| button == Input::Keycode::JS_POV_XAXIS_NEG
			#ifdef CONFIG_ENV_WEBOS
			|| button == Input::Keycode::D
			#endif
			;
	}
	return false;
}

bool Event::isDefaultRightButton() const
{
	switch(map())
	{
		#ifdef CONFIG_BLUETOOTH
		case MAP_WIIMOTE:
			return button == Input::Wiimote::RIGHT || button == Input::Wiimote::NUN_STICK_RIGHT;
		case MAP_WII_CC:
			return button == Input::WiiCC::RIGHT || button == Input::WiiCC::LSTICK_RIGHT;
		case MAP_ICONTROLPAD: return button == Input::iControlPad::RIGHT || button == Input::iControlPad::LNUB_RIGHT;
		case MAP_ZEEMOTE: return button == Input::Zeemote::RIGHT;
		#endif
		case MAP_ICADE: return button == Input::ICade::RIGHT;
		#if defined CONFIG_BASE_PS3 || defined CONFIG_BLUETOOTH
		case MAP_PS3PAD: return button == Input::PS3::RIGHT || button == PS3::LSTICK_RIGHT;
		#endif
		#ifdef CONFIG_INPUT_APPLE_GAME_CONTROLLER
		case MAP_APPLE_GAME_CONTROLLER: return button == Input::AppleGC::RIGHT;
		#endif
		case MAP_SYSTEM:
			return button == Input::Keycode::RIGHT
			|| button == Input::Keycode::JS1_XAXIS_POS
			|| button == Input::Keycode::JS_POV_XAXIS_POS
			#ifdef CONFIG_ENV_WEBOS
			|| button == Input::Keycode::G
			#endif
			;
	}
	return 0;
}

bool Event::isDefaultUpButton() const
{
	switch(map())
	{
		#ifdef CONFIG_BLUETOOTH
		case MAP_WIIMOTE:
			return button == Input::Wiimote::UP || button == Input::Wiimote::NUN_STICK_UP;
		case MAP_WII_CC:
			return button == Input::WiiCC::UP || button == Input::WiiCC::LSTICK_UP;
		case MAP_ICONTROLPAD: return button == Input::iControlPad::UP || button == Input::iControlPad::LNUB_UP;
		case MAP_ZEEMOTE: return button == Input::Zeemote::UP;
		#endif
		case MAP_ICADE: return button == Input::ICade::UP;
		#if defined CONFIG_BASE_PS3 || defined CONFIG_BLUETOOTH
		case MAP_PS3PAD: return button == Input::PS3::UP || button == PS3::LSTICK_UP;
		#endif
		#ifdef CONFIG_INPUT_APPLE_GAME_CONTROLLER
		case MAP_APPLE_GAME_CONTROLLER: return button == Input::AppleGC::UP;
		#endif
		case MAP_SYSTEM:
			return button == Input::Keycode::UP
			|| button == Input::Keycode::JS1_YAXIS_NEG
			|| button == Input::Keycode::JS_POV_YAXIS_NEG
			#ifdef CONFIG_ENV_WEBOS
			|| button == Input::Keycode::R
			#endif
			;
	}
	return false;
}

bool Event::isDefaultDownButton() const
{
	switch(map())
	{
		#ifdef CONFIG_BLUETOOTH
		case MAP_WIIMOTE:
			return button == Input::Wiimote::DOWN || button == Input::Wiimote::NUN_STICK_DOWN;
		case MAP_WII_CC:
			return button == Input::WiiCC::DOWN || button == Input::WiiCC::LSTICK_DOWN;
		case MAP_ICONTROLPAD: return button == Input::iControlPad::DOWN || button == Input::iControlPad::LNUB_DOWN;
		case MAP_ZEEMOTE: return button == Input::Zeemote::DOWN;
		#endif
		case MAP_ICADE: return button == Input::ICade::DOWN;
		#if defined CONFIG_BASE_PS3 || defined CONFIG_BLUETOOTH
		case MAP_PS3PAD: return button == Input::PS3::DOWN || button == PS3::LSTICK_DOWN;
		#endif
		#ifdef CONFIG_INPUT_APPLE_GAME_CONTROLLER
		case MAP_APPLE_GAME_CONTROLLER: return button == Input::AppleGC::DOWN;
		#endif
		case MAP_SYSTEM:
			return button == Input::Keycode::DOWN
			|| button == Input::Keycode::JS1_YAXIS_POS
			|| button == Input::Keycode::JS_POV_YAXIS_POS
			#ifdef CONFIG_ENV_WEBOS
			|| button == Input::Keycode::C
			#endif
			;
	}
	return false;
}

bool Event::isDefaultDirectionButton() const
{
	return isDefaultLeftButton() || isDefaultRightButton() || isDefaultUpButton() || isDefaultDownButton();
}

bool Event::isDefaultPageUpButton() const
{
	switch(map())
	{
		#ifdef CONFIG_BLUETOOTH
		case MAP_WIIMOTE: return button == Input::Wiimote::PLUS;
		case MAP_WII_CC: return button == Input::WiiCC::L;
		case MAP_ICONTROLPAD: return button == Input::iControlPad::L;
		case MAP_ZEEMOTE: return 0;
		#endif
		case MAP_ICADE: return button == Input::ICade::Z;
		#if defined CONFIG_BASE_PS3 || defined CONFIG_BLUETOOTH
		case MAP_PS3PAD: return button == Input::PS3::L1;
		#endif
		#ifdef CONFIG_INPUT_APPLE_GAME_CONTROLLER
		case MAP_APPLE_GAME_CONTROLLER: return button == Input::AppleGC::L1;
		#endif
		case MAP_SYSTEM:
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
	return false;
}

bool Event::isDefaultPageDownButton() const
{
	switch(map())
	{
		#ifdef CONFIG_BLUETOOTH
		case MAP_WIIMOTE: return button == Input::Wiimote::MINUS;
		case MAP_WII_CC: return button == Input::WiiCC::R;
		case MAP_ICONTROLPAD: return button == Input::iControlPad::R;
		case MAP_ZEEMOTE: return 0;
		#endif
		case MAP_ICADE: return button == Input::ICade::C;
		#if defined CONFIG_BASE_PS3 || defined CONFIG_BLUETOOTH
		case MAP_PS3PAD: return button == Input::PS3::R1;
		#endif
		#ifdef CONFIG_INPUT_APPLE_GAME_CONTROLLER
		case MAP_APPLE_GAME_CONTROLLER: return button == Input::AppleGC::R1;
		#endif
		case MAP_SYSTEM:
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
	return false;
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
	return state() == PUSHED;
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
	return state() == RELEASED;
}

bool Event::released(Key key) const
{
	return released() && button == key;
}

bool Event::releasedKey(Key sysKey) const
{
	return released() && sysKey_ == sysKey;
}

bool Event::moved() const
{
	return state() == MOVED;
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

const char *Event::actionToStr(int action)
{
	switch(action)
	{
		default:
		case UNUSED: return "Unknown";
		case RELEASED: return "Released";
		case PUSHED: return "Pushed";
		case MOVED: return "Moved";
		case MOVED_RELATIVE: return "Moved Relative";
		case EXIT_VIEW: return "Left View";
		case ENTER_VIEW: return "Entered View";
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

}
