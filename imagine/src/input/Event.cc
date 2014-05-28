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

namespace Input
{

bool Event::isDefaultConfirmButton(uint swapped) const
{
	switch(map)
	{
		#ifdef CONFIG_BLUETOOTH
		case MAP_WIIMOTE: return swapped ? isDefaultCancelButton(0) :
				(button == Input::Wiimote::_1 || button == Input::Wiimote::NUN_Z);
		case MAP_WII_CC: return swapped ? isDefaultCancelButton(0) : button == Input::WiiCC::B;
		case MAP_ICONTROLPAD: return swapped ? isDefaultCancelButton(0) : (button == Input::iControlPad::X);
		case MAP_ZEEMOTE: return swapped ? isDefaultCancelButton(0) : (button == Input::Zeemote::A);
		#endif
		#ifdef CONFIG_INPUT_ICADE
		case MAP_ICADE: return swapped ? isDefaultCancelButton(0) : (button == Input::ICade::A || button == Input::ICade::B);
		#endif
		#if defined CONFIG_BASE_PS3 || defined CONFIG_BLUETOOTH
		case MAP_PS3PAD: return swapped ? isDefaultCancelButton(0) : (button == Input::PS3::CROSS);
		#endif
		#ifdef CONFIG_INPUT_EVDEV
		case MAP_EVDEV: return button == Input::Evdev::GAME_A || button == Input::Evdev::GAME_1;
		#endif
		#ifdef CONFIG_INPUT_APPLE_GAME_CONTROLLER
		case MAP_APPLE_GAME_CONTROLLER: return button == Input::AppleGC::A;
		#endif
		#ifdef INPUT_SUPPORTS_KEYBOARD
		case MAP_SYSTEM:
			switch(device->subtype())
			{
				#ifdef CONFIG_BASE_ANDROID
				case Device::SUBTYPE_PS3_CONTROLLER:
					return swapped ? isDefaultCancelButton(0) : button == Input::Keycode::GAME_X;
				#endif
				#ifdef CONFIG_MACHINE_PANDORA
				case Device::SUBTYPE_PANDORA_HANDHELD:
					return button == Input::Keycode::ENTER ||
						(swapped ? isDefaultCancelButton(0) : button == Keycode::Pandora::X);
				#endif
			}
			return button == Input::Keycode::ENTER
			#ifdef CONFIG_BASE_ANDROID
			|| button == Input::Keycode::CENTER || (swapped ? isDefaultCancelButton(0) : (button == Input::Keycode::GAME_A || button == Input::Keycode::GAME_1))
			#endif
			;
		#endif
	}
	return false;
}

bool Event::isDefaultCancelButton(uint swapped) const
{
	switch(map)
	{
		#ifdef CONFIG_BLUETOOTH
		case MAP_WIIMOTE: return swapped ? isDefaultConfirmButton(0) :
				(button == Input::Wiimote::_2 || button == Input::Wiimote::NUN_C);
		case MAP_WII_CC: return swapped ? isDefaultConfirmButton(0) : button == Input::WiiCC::A;
		case MAP_ICONTROLPAD: return swapped ? isDefaultConfirmButton(0) : (button == Input::iControlPad::B);
		case MAP_ZEEMOTE: return swapped ? isDefaultConfirmButton(0) : (button == Input::Zeemote::B);
		#endif
		#ifdef CONFIG_INPUT_ICADE
		case MAP_ICADE: return swapped ? isDefaultConfirmButton(0) : (button == Input::ICade::C || button == Input::ICade::D);
		#endif
		#if defined CONFIG_BASE_PS3 || defined CONFIG_BLUETOOTH
		case MAP_PS3PAD: return swapped ? isDefaultConfirmButton(0) : (button == Input::PS3::CIRCLE);
		#endif
		#ifdef INPUT_SUPPORTS_MOUSE
		case MAP_POINTER: return button == Input::Pointer::DOWN_BUTTON;
		#endif
		#ifdef CONFIG_INPUT_EVDEV
		case MAP_EVDEV: return button == Input::Evdev::GAME_B || button == Input::Evdev::GAME_2;
		#endif
		#ifdef CONFIG_INPUT_APPLE_GAME_CONTROLLER
		case MAP_APPLE_GAME_CONTROLLER: return button == Input::AppleGC::B;
		#endif
		#ifdef INPUT_SUPPORTS_KEYBOARD
		case MAP_SYSTEM:
			switch(device->subtype())
			{
				#ifdef CONFIG_BASE_ANDROID
				case Device::SUBTYPE_PS3_CONTROLLER:
					return swapped ? isDefaultConfirmButton(0) : button == Input::Keycode::GAME_Y;
				#endif
				#ifdef CONFIG_MACHINE_PANDORA
				case Device::SUBTYPE_PANDORA_HANDHELD:
					// TODO: can't call isDefaultConfirmButton(0) since it doesn't check whether the source was
					// a gamepad or keyboard
					return swapped ? (button == Keycode::Pandora::X) : (button == Keycode::Pandora::B);
				#endif
			}
			return button == Input::Keycode::ESCAPE
				#ifdef CONFIG_INPUT_ANDROID
				|| (swapped ? isDefaultConfirmButton(0) : (button == Input::Keycode::GAME_B || button == Input::Keycode::GAME_2))
				#endif
				;
		#endif
	}
	return false;
}

bool Event::isDefaultLeftButton() const
{
	switch(map)
	{
		#ifdef CONFIG_BLUETOOTH
		case MAP_WIIMOTE:
			return button == Input::Wiimote::LEFT || button == Input::Wiimote::NUN_STICK_LEFT;
		case MAP_WII_CC:
			return button == Input::WiiCC::LEFT || button == Input::WiiCC::LSTICK_LEFT;
		case MAP_ICONTROLPAD: return button == Input::iControlPad::LEFT || button == Input::iControlPad::LNUB_LEFT;
		case MAP_ZEEMOTE: return button == Input::Zeemote::LEFT;
		#endif
		#ifdef CONFIG_INPUT_ICADE
		case MAP_ICADE: return button == Input::ICade::LEFT;
		#endif
		#if defined CONFIG_BASE_PS3 || defined CONFIG_BLUETOOTH
		case MAP_PS3PAD: return button == Input::PS3::LEFT || button == PS3::LSTICK_LEFT;
		#endif
		#ifdef CONFIG_INPUT_EVDEV
		case MAP_EVDEV:
			return button == Input::Evdev::LEFT || button == Input::Evdev::JS1_XAXIS_NEG || button == Input::Evdev::JS_POV_XAXIS_NEG;
		#endif
		#ifdef CONFIG_INPUT_APPLE_GAME_CONTROLLER
		case MAP_APPLE_GAME_CONTROLLER: return button == Input::AppleGC::LEFT;
		#endif
		#ifdef INPUT_SUPPORTS_KEYBOARD
		case MAP_SYSTEM:
			return button == Input::Keycode::LEFT
			#ifdef CONFIG_BASE_ANDROID
			|| button == Input::Keycode::JS1_XAXIS_NEG
			|| button == Input::Keycode::JS_POV_XAXIS_NEG
			#endif
			#ifdef CONFIG_ENV_WEBOS
			|| button == Input::asciiKey('d')
			#endif
			;
		#endif
	}
	return false;
}

bool Event::isDefaultRightButton() const
{
	switch(map)
	{
		#ifdef CONFIG_BLUETOOTH
		case MAP_WIIMOTE:
			return button == Input::Wiimote::RIGHT || button == Input::Wiimote::NUN_STICK_RIGHT;
		case MAP_WII_CC:
			return button == Input::WiiCC::RIGHT || button == Input::WiiCC::LSTICK_RIGHT;
		case MAP_ICONTROLPAD: return button == Input::iControlPad::RIGHT || button == Input::iControlPad::LNUB_RIGHT;
		case MAP_ZEEMOTE: return button == Input::Zeemote::RIGHT;
		#endif
		#ifdef CONFIG_INPUT_ICADE
		case MAP_ICADE: return button == Input::ICade::RIGHT;
		#endif
		#if defined CONFIG_BASE_PS3 || defined CONFIG_BLUETOOTH
		case MAP_PS3PAD: return button == Input::PS3::RIGHT || button == PS3::LSTICK_RIGHT;
		#endif
		#ifdef CONFIG_INPUT_EVDEV
		case MAP_EVDEV:
			return button == Input::Evdev::RIGHT || button == Input::Evdev::JS1_XAXIS_POS || button == Input::Evdev::JS_POV_XAXIS_POS;
		#endif
		#ifdef CONFIG_INPUT_APPLE_GAME_CONTROLLER
		case MAP_APPLE_GAME_CONTROLLER: return button == Input::AppleGC::RIGHT;
		#endif
		#ifdef INPUT_SUPPORTS_KEYBOARD
		case MAP_SYSTEM:
			return button == Input::Keycode::RIGHT
			#ifdef CONFIG_BASE_ANDROID
			|| button == Input::Keycode::JS1_XAXIS_POS
			|| button == Input::Keycode::JS_POV_XAXIS_POS
			#endif
			#ifdef CONFIG_ENV_WEBOS
			|| button == Input::asciiKey('g')
			#endif
			;
		#endif
	}
	return 0;
}

bool Event::isDefaultUpButton() const
{
	switch(map)
	{
		#ifdef CONFIG_BLUETOOTH
		case MAP_WIIMOTE:
			return button == Input::Wiimote::UP || button == Input::Wiimote::NUN_STICK_UP;
		case MAP_WII_CC:
			return button == Input::WiiCC::UP || button == Input::WiiCC::LSTICK_UP;
		case MAP_ICONTROLPAD: return button == Input::iControlPad::UP || button == Input::iControlPad::LNUB_UP;
		case MAP_ZEEMOTE: return button == Input::Zeemote::UP;
		#endif
		#ifdef CONFIG_INPUT_ICADE
		case MAP_ICADE: return button == Input::ICade::UP;
		#endif
		#if defined CONFIG_BASE_PS3 || defined CONFIG_BLUETOOTH
		case MAP_PS3PAD: return button == Input::PS3::UP || button == PS3::LSTICK_UP;
		#endif
		#ifdef CONFIG_INPUT_EVDEV
		case MAP_EVDEV:
			return button == Input::Evdev::UP || button == Input::Evdev::JS1_YAXIS_NEG || button == Input::Evdev::JS_POV_YAXIS_NEG;
		#endif
		#ifdef CONFIG_INPUT_APPLE_GAME_CONTROLLER
		case MAP_APPLE_GAME_CONTROLLER: return button == Input::AppleGC::UP;
		#endif
		#ifdef INPUT_SUPPORTS_KEYBOARD
		case MAP_SYSTEM:
			return button == Input::Keycode::UP
			#ifdef CONFIG_BASE_ANDROID
			|| button == Input::Keycode::JS1_YAXIS_NEG
			|| button == Input::Keycode::JS_POV_YAXIS_NEG
			#endif
			#ifdef CONFIG_ENV_WEBOS
			|| button == Input::asciiKey('r')
			#endif
			;
		#endif
	}
	return false;
}

bool Event::isDefaultDownButton() const
{
	switch(map)
	{
		#ifdef CONFIG_BLUETOOTH
		case MAP_WIIMOTE:
			return button == Input::Wiimote::DOWN || button == Input::Wiimote::NUN_STICK_DOWN;
		case MAP_WII_CC:
			return button == Input::WiiCC::DOWN || button == Input::WiiCC::LSTICK_DOWN;
		case MAP_ICONTROLPAD: return button == Input::iControlPad::DOWN || button == Input::iControlPad::LNUB_DOWN;
		case MAP_ZEEMOTE: return button == Input::Zeemote::DOWN;
		#endif
		#ifdef CONFIG_INPUT_ICADE
		case MAP_ICADE: return button == Input::ICade::DOWN;
		#endif
		#if defined CONFIG_BASE_PS3 || defined CONFIG_BLUETOOTH
		case MAP_PS3PAD: return button == Input::PS3::DOWN || button == PS3::LSTICK_DOWN;
		#endif
		#ifdef CONFIG_INPUT_EVDEV
		case MAP_EVDEV:
			return button == Input::Evdev::DOWN || button == Input::Evdev::JS1_YAXIS_POS|| button == Input::Evdev::JS_POV_YAXIS_POS;
		#endif
		#ifdef CONFIG_INPUT_APPLE_GAME_CONTROLLER
		case MAP_APPLE_GAME_CONTROLLER: return button == Input::AppleGC::DOWN;
		#endif
		#ifdef INPUT_SUPPORTS_KEYBOARD
		case MAP_SYSTEM:
			return button == Input::Keycode::DOWN
			#ifdef CONFIG_BASE_ANDROID
			|| button == Input::Keycode::JS1_YAXIS_POS
			|| button == Input::Keycode::JS_POV_YAXIS_POS
			#endif
			#ifdef CONFIG_ENV_WEBOS
			|| button == Input::asciiKey('c')
			#endif
			;
		#endif
	}
	return false;
}

bool Event::isDefaultPageUpButton() const
{
	switch(map)
	{
		#ifdef CONFIG_BLUETOOTH
		case MAP_WIIMOTE: return button == Input::Wiimote::PLUS;
		case MAP_WII_CC: return button == Input::WiiCC::L;
		case MAP_ICONTROLPAD: return button == Input::iControlPad::L;
		case MAP_ZEEMOTE: return 0;
		#endif
		#ifdef CONFIG_INPUT_ICADE
		case MAP_ICADE: return button == Input::ICade::E;
		#endif
		#if defined CONFIG_BASE_PS3 || defined CONFIG_BLUETOOTH
		case MAP_PS3PAD: return button == Input::PS3::L1;
		#endif
		#ifdef CONFIG_INPUT_EVDEV
		case MAP_EVDEV: return button == Input::Evdev::GAME_L1;
		#endif
		#ifdef CONFIG_INPUT_APPLE_GAME_CONTROLLER
		case MAP_APPLE_GAME_CONTROLLER: return button == Input::AppleGC::L1;
		#endif
		#ifdef INPUT_SUPPORTS_KEYBOARD
		case MAP_SYSTEM:
			switch(device->subtype())
			{
				#ifdef CONFIG_MACHINE_PANDORA
				case Device::SUBTYPE_PANDORA_HANDHELD:
					return button == Keycode::Pandora::L;
				#endif
			}
			return button == Input::Keycode::PGUP
		#ifdef CONFIG_BASE_ANDROID
				|| button == Input::Keycode::GAME_L1
		#endif
				;
		#endif
	}
	return false;
}

bool Event::isDefaultPageDownButton() const
{
	switch(map)
	{
		#ifdef CONFIG_BLUETOOTH
		case MAP_WIIMOTE: return button == Input::Wiimote::MINUS;
		case MAP_WII_CC: return button == Input::WiiCC::R;
		case MAP_ICONTROLPAD: return button == Input::iControlPad::R;
		case MAP_ZEEMOTE: return 0;
		#endif
		#ifdef CONFIG_INPUT_ICADE
		case MAP_ICADE: return button == Input::ICade::F;
		#endif
		#if defined CONFIG_BASE_PS3 || defined CONFIG_BLUETOOTH
		case MAP_PS3PAD: return button == Input::PS3::R1;
		#endif
		#ifdef CONFIG_INPUT_EVDEV
		case MAP_EVDEV: return button == Input::Evdev::GAME_R1;
		#endif
		#ifdef CONFIG_INPUT_APPLE_GAME_CONTROLLER
		case MAP_APPLE_GAME_CONTROLLER: return button == Input::AppleGC::R1;
		#endif
		#ifdef INPUT_SUPPORTS_KEYBOARD
		case MAP_SYSTEM:
			switch(device->subtype())
			{
				#ifdef CONFIG_MACHINE_PANDORA
				case Device::SUBTYPE_PANDORA_HANDHELD:
					return button == Keycode::Pandora::R;
				#endif
			}
			return button == Input::Keycode::PGDOWN
		#ifdef CONFIG_BASE_ANDROID
				|| button == Input::Keycode::GAME_R1
		#endif
				;
		#endif
	}
	return false;
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

}
