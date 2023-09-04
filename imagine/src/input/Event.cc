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

#include <imagine/input/Event.hh>
#include <imagine/input/Device.hh>
#include <imagine/base/Application.hh>

namespace IG::Input
{

std::string_view BaseEvent::mapName() const
{
	return mapName(map());
}

Map BaseEvent::map() const
{
	return map_;
}

void BaseEvent::setMap(Map map)
{
	map_ = map;
}

PointerId MotionEvent::pointerId() const
{
	return pointerId_;
}

Action BaseEvent::state() const
{
	return state_;
}

void KeyEvent::setKeyFlags(uint8_t flags)
{
	keyFlags = flags;
}

bool MotionEvent::isAbsolute() const
{
	return Config::Input::POINTING_DEVICES && map() == Map::POINTER;
}

bool MotionEvent::isRelative() const
{
	return Config::Input::RELATIVE_MOTION_DEVICES && map() == Map::REL_POINTER;
}

bool MotionEvent::isTouch() const
{
	return Config::Input::TOUCH_DEVICES && src == Source::TOUCHSCREEN;
}

bool KeyEvent::isGamepad() const
{
	return src == Source::GAMEPAD;
}

bool KeyEvent::isKeyboard() const
{
	return src == Source::KEYBOARD;
}

bool KeyEvent::isDefaultConfirmButton(uint32_t swapped) const
{
	switch(map())
	{
		default: return false;
		#ifdef CONFIG_INPUT_BLUETOOTH
		case Map::WIIMOTE: return swapped ? isDefaultCancelButton(0) :
				(button == Input::Wiimote::_1 || button == Input::Wiimote::NUN_Z);
		case Map::WII_CC:
		case Map::ICONTROLPAD:
		case Map::ZEEMOTE:
		#endif
		#ifdef CONFIG_BLUETOOTH_SERVER
		case Map::PS3PAD:
		#endif
		#ifdef CONFIG_INPUT_APPLE_GAME_CONTROLLER
		case Map::APPLE_GAME_CONTROLLER:
		#endif
		case Map::ICADE:
			return swapped ? isDefaultCancelButton(0) : sysKey_ == Keycode::GAME_A;
		case Map::SYSTEM:
			switch(device()->subtype())
			{
				#ifdef CONFIG_MACHINE_PANDORA
				case Device::Subtype::PANDORA_HANDHELD:
					return button == Input::Keycode::ENTER ||
						(swapped ? isDefaultCancelButton(0) : button == Keycode::Pandora::X);
				#endif
				default: break;
			}
			return button == Keycode::ENTER
			#ifdef __ANDROID__
			|| button == Keycode::CENTER
			#endif
			|| ((swapped && isGamepad()) ? isDefaultCancelButton(0) : (button == Keycode::GAME_A || button == Keycode::GAME_1));
	}
}

bool KeyEvent::isDefaultCancelButton(uint32_t swapped) const
{
	switch(map())
	{
		default: return false;
		#ifdef CONFIG_INPUT_BLUETOOTH
		case Map::WIIMOTE: return swapped ? isDefaultConfirmButton(0) :
				(button == Input::Wiimote::_2 || button == Input::Wiimote::NUN_C);
		case Map::WII_CC:
		case Map::ICONTROLPAD:
		case Map::ZEEMOTE:
		#endif
		#ifdef CONFIG_BLUETOOTH_SERVER
		case Map::PS3PAD:
		#endif
		#ifdef CONFIG_INPUT_APPLE_GAME_CONTROLLER
		case Map::APPLE_GAME_CONTROLLER:
		#endif
		case Map::ICADE:
			return swapped ? isDefaultConfirmButton(0) : sysKey_ == Keycode::GAME_B;
		#ifdef CONFIG_INPUT_MOUSE_DEVICES
		case Map::POINTER: return button == Pointer::DOWN_BUTTON;
		#endif
		case Map::SYSTEM:
			switch(device()->subtype())
			{
				#ifdef CONFIG_MACHINE_PANDORA
				case Device::Subtype::PANDORA_HANDHELD:
					// TODO: can't call isDefaultConfirmButton(0) since it doesn't check whether the source was
					// a gamepad or keyboard
					return swapped ? (button == Keycode::Pandora::X) : (button == Keycode::Pandora::B);
				#endif
				default: break;
			}
			return button == Input::Keycode::ESCAPE || button == Input::Keycode::BACK
				|| ((swapped && isGamepad()) ? isDefaultConfirmButton(0) : (button == Input::Keycode::GAME_B || button == Input::Keycode::GAME_2));
	}
}

bool KeyEvent::isDefaultConfirmButton() const
{
	return isDefaultConfirmButton(hasSwappedConfirmKeys());
}

bool KeyEvent::isDefaultCancelButton() const
{
	return isDefaultCancelButton(hasSwappedConfirmKeys());
}

bool KeyEvent::isDefaultLeftButton() const
{
	switch(map())
	{
		default: return false;
		#ifdef CONFIG_INPUT_BLUETOOTH
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

bool KeyEvent::isDefaultRightButton() const
{
	switch(map())
	{
		default: return false;
		#ifdef CONFIG_INPUT_BLUETOOTH
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

bool KeyEvent::isDefaultUpButton() const
{
	switch(map())
	{
		default: return false;
		#ifdef CONFIG_INPUT_BLUETOOTH
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

bool KeyEvent::isDefaultDownButton() const
{
	switch(map())
	{
		default: return false;
		#ifdef CONFIG_INPUT_BLUETOOTH
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

bool KeyEvent::isDefaultDirectionButton() const
{
	return isDefaultLeftButton() || isDefaultRightButton() || isDefaultUpButton() || isDefaultDownButton();
}

bool KeyEvent::isDefaultPageUpButton() const
{
	switch(map())
	{
		default: return false;
		#ifdef CONFIG_INPUT_BLUETOOTH
		case Map::WIIMOTE: return button == Input::Wiimote::PLUS;
		case Map::WII_CC: return button == Input::WiiCC::L;
		case Map::ICONTROLPAD: return button == Input::iControlPad::L;
		case Map::ZEEMOTE: return false;
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
				case Device::Subtype::PANDORA_HANDHELD:
					return button == Keycode::Pandora::L;
				#endif
				default: break;
			}
			return button == Input::Keycode::PGUP
				|| button == Input::Keycode::GAME_L1;
	}
}

bool KeyEvent::isDefaultPageDownButton() const
{
	switch(map())
	{
		default: return false;
		#ifdef CONFIG_INPUT_BLUETOOTH
		case Map::WIIMOTE: return button == Input::Wiimote::MINUS;
		case Map::WII_CC: return button == Input::WiiCC::R;
		case Map::ICONTROLPAD: return button == Input::iControlPad::R;
		case Map::ZEEMOTE: return false;
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
				case Device::Subtype::PANDORA_HANDHELD:
					return button == Keycode::Pandora::R;
				#endif
				default: break;
			}
			return button == Input::Keycode::PGDOWN
				|| button == Input::Keycode::GAME_R1;
	}
}

bool KeyEvent::isDefaultKey(DefaultKey dKey) const
{
	switch(dKey)
	{
		case DefaultKey::CONFIRM: return isDefaultConfirmButton();
		case DefaultKey::CANCEL: return isDefaultCancelButton();
		case DefaultKey::LEFT: return isDefaultLeftButton();
		case DefaultKey::RIGHT: return isDefaultRightButton();
		case DefaultKey::UP: return isDefaultUpButton();
		case DefaultKey::DOWN: return isDefaultDownButton();
		case DefaultKey::DIRECTION: return isDefaultDirectionButton();
		case DefaultKey::PAGE_UP: return isDefaultPageUpButton();
		case DefaultKey::PAGE_DOWN: return isDefaultPageDownButton();
	}
	return false;
}

Key KeyEvent::key() const
{
	return sysKey_;
}

Key BaseEvent::mapKey() const
{
	return button;
}

#ifdef CONFIG_BASE_X11
void KeyEvent::setX11RawKey(Key key)
{
	rawKey = key;
}
#endif

bool BaseEvent::pushed(Key key) const
{
	return state() == Action::PUSHED
		&& (!key || button == key);
}

bool KeyEvent::pushedKey(Key sysKey) const
{
	return pushed() && sysKey_ == sysKey;
}

bool BaseEvent::released(Key key) const
{
	return state() == Action::RELEASED
		&& (!key || button == key);
}

bool KeyEvent::releasedKey(Key sysKey) const
{
	return released() && sysKey_ == sysKey;
}

bool KeyEvent::pushed(DefaultKey defaultKey) const
{
	return pushed() && isDefaultKey(defaultKey);
}

bool KeyEvent::released(DefaultKey defaultKey) const
{
	return released() && isDefaultKey(defaultKey);
}

bool MotionEvent::canceled() const
{
	return state() == Action::CANCELED;
}

bool MotionEvent::isOff() const
{
	return released() || canceled();
}

bool MotionEvent::moved() const
{
	return state() == Action::MOVED;
}

uint32_t BaseEvent::metaKeyBits() const
{
	return metaState;
}

bool BaseEvent::isShiftPushed() const
{
	return metaState & Meta::SHIFT;
}

int KeyEvent::repeated() const
{
	return repeatCount;
}

void KeyEvent::setRepeatCount(int count)
{
	repeatCount = count;
}

WPt MotionEvent::pos() const
{
	return {x, y};
}

bool MotionEvent::pointerDown(Key btnMask) const
{
	return !released() && button & btnMask;
}

int MotionEvent::scrolledVertical() const
{
	switch(state())
	{
		case Action::SCROLL_UP: return -1;
		case Action::SCROLL_DOWN: return 1;
		default: return 0;
	}
}

bool KeyEvent::isSystemFunction() const
{
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

std::string_view BaseEvent::actionToStr(Action action)
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

SteadyClockTimePoint BaseEvent::time() const
{
	return time_;
}

const Device *BaseEvent::device() const
{
	return device_;
}

bool KeyEvent::hasSwappedConfirmKeys() const
{
	return keyFlags; // currently there is only a single flag
}

SteadyClockTimePoint Event::time() const { return visit([](auto &e){ return e.time(); }, *this); }

const Device *Event::device() const { return visit([](auto &e){ return e.device(); }, *this); }

}
