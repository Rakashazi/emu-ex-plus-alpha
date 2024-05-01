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
#include <imagine/input/bluetoothInputDefs.hh>
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

bool MotionEvent::isPointer() const
{
	return Config::Input::POINTING_DEVICES && map() == Map::POINTER;
}

bool MotionEvent::isJoystick() const
{
	return src == Source::JOYSTICK;
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
	switch(device()->subtype())
	{
		#ifdef CONFIG_MACHINE_PANDORA
		case DeviceSubtype::PANDORA_HANDHELD:
			return key() == Input::Keycode::ENTER ||
				(swapped ? isDefaultCancelButton(0) : key() == Keycode::Pandora::X);
		#endif
		default: break;
	}
	return key() == Keycode::ENTER
	#ifdef __ANDROID__
	|| key() == Keycode::CENTER
	#endif
	|| ((swapped && isGamepad()) ? isDefaultCancelButton(0) : (key() == Keycode::GAME_A || key() == Keycode::GAME_C));
}

bool KeyEvent::isDefaultCancelButton(uint32_t swapped) const
{
	switch(device()->subtype())
	{
		#ifdef CONFIG_MACHINE_PANDORA
		case DeviceSubtype::PANDORA_HANDHELD:
			// TODO: can't call isDefaultConfirmButton(0) since it doesn't check whether the source was
			// a gamepad or keyboard
			return swapped ? (key() == Keycode::Pandora::X) : (key() == Keycode::Pandora::B);
		#endif
		default: break;
	}
	if(Config::Input::MOUSE_DEVICES && key() == Pointer::DOWN_BUTTON)
		return true;
	return key() == Input::Keycode::ESCAPE || key() == Input::Keycode::BACK
		|| ((swapped && isGamepad()) ? isDefaultConfirmButton(0) : (key() == Input::Keycode::GAME_B || key() == Input::Keycode::GAME_Z));
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
	return key() == Input::Keycode::LEFT
	|| key() == Input::Keycode::JS1_XAXIS_NEG
	|| key() == Input::Keycode::JS_POV_XAXIS_NEG
	;
}

bool KeyEvent::isDefaultRightButton() const
{
	return key() == Input::Keycode::RIGHT
	|| key() == Input::Keycode::JS1_XAXIS_POS
	|| key() == Input::Keycode::JS_POV_XAXIS_POS
	;
}

bool KeyEvent::isDefaultUpButton() const
{
	return key() == Input::Keycode::UP
	|| key() == Input::Keycode::JS1_YAXIS_NEG
	|| key() == Input::Keycode::JS_POV_YAXIS_NEG
	;
}

bool KeyEvent::isDefaultDownButton() const
{
	return key() == Input::Keycode::DOWN
	|| key() == Input::Keycode::JS1_YAXIS_POS
	|| key() == Input::Keycode::JS_POV_YAXIS_POS
	;
}

bool KeyEvent::isDefaultDirectionButton() const
{
	return isDefaultLeftButton() || isDefaultRightButton() || isDefaultUpButton() || isDefaultDownButton();
}

bool KeyEvent::isDefaultPageUpButton() const
{
	switch(map())
	{
		#ifdef CONFIG_INPUT_BLUETOOTH
		case Map::WIIMOTE: return key() == Input::WiimoteKey::PLUS;
		#endif
		default:
			switch(device()->subtype())
			{
				#ifdef CONFIG_MACHINE_PANDORA
				case DeviceSubtype::PANDORA_HANDHELD:
					return key() == Keycode::Pandora::L;
				#endif
				default: break;
			}
			return key() == Input::Keycode::PGUP
				|| key() == Input::Keycode::GAME_L1;
	}
}

bool KeyEvent::isDefaultPageDownButton() const
{
	switch(map())
	{
		#ifdef CONFIG_INPUT_BLUETOOTH
		case Map::WIIMOTE: return key() == Input::WiimoteKey::MINUS;
		#endif
		default:
			switch(device()->subtype())
			{
				#ifdef CONFIG_MACHINE_PANDORA
				case DeviceSubtype::PANDORA_HANDHELD:
					return key() == Keycode::Pandora::R;
				#endif
				default: break;
			}
			return key() == Input::Keycode::PGDOWN
				|| key() == Input::Keycode::GAME_R1;
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

Key BaseEvent::key() const
{
	return key_;
}

#if CONFIG_PACKAGE_X11
void KeyEvent::setX11RawKey(Key key)
{
	rawKey = key;
}
#endif

bool BaseEvent::pushed() const { return state() == Action::PUSHED; }

bool BaseEvent::pushed(Key key) const
{
	return pushed() && key_ == key;
}

bool BaseEvent::released() const { return state() == Action::RELEASED; }

bool BaseEvent::released(Key key) const
{
	return released() && key_ == key;
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

bool MotionEvent::pointerDown(Key btnMask) const
{
	return !released() && key() & btnMask;
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

std::string_view BaseEvent::toString(Action action)
{
	switch(action)
	{
		case Action::UNUSED: break;
		case Action::RELEASED: return "Released";
		case Action::PUSHED: return "Pushed";
		case Action::MOVED: return "Moved";
		case Action::MOVED_RELATIVE: return "Moved Relative";
		case Action::EXIT_VIEW: return "Left View";
		case Action::ENTER_VIEW: return "Entered View";
		case Action::SCROLL_UP: return "Scrolled Up";
		case Action::SCROLL_DOWN: return "Scrolled Down";
		case Action::CANCELED: return "Canceled";
	}
	return "Unknown";
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

SteadyClockTimePoint Event::time() const { return visit([](auto &e){ return e.time(); }); }

const Device *Event::device() const { return visit([](auto &e){ return e.device(); }); }

}
