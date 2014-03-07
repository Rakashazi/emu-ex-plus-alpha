#pragma once

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

#include <input/Input.hh>
#include <gfx/Gfx.hh>
#include <base/Timer.hh>

#ifdef CONFIG_BLUETOOTH
#include <bluetooth/BluetoothInputDevScanner.hh>
#endif

namespace Input
{

static Base::Timer keyRepeatTimer;
static Event keyRepeatEvent;
static bool allowKeyRepeats = true;

void startKeyRepeatTimer(const Event &event)
{
	if(!allowKeyRepeats)
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
	keyRepeatTimer.callbackAfterMSec(
		[]()
		{
			logMsg("repeating key event");
			if(likely(keyRepeatEvent.pushed()))
				Base::onInputEvent(Base::mainWindow(), keyRepeatEvent);
		}, 400, 50, Base::Timer::HINT_REUSE);
}

void cancelKeyRepeatTimer()
{
	//logMsg("cancelled key repeat");
	keyRepeatTimer.cancel();
	keyRepeatEvent = {};
}

void deinitKeyRepeatTimer()
{
	//logMsg("deinit key repeat");
	keyRepeatTimer.deinit();
	keyRepeatEvent = {};
}

StaticArrayList<Device*, MAX_DEVS> devList;

void addDevice(Device &d)
{
	d.idx = devList.size();
	devList.push_back(&d);
}

void removeDevice(Device &d)
{
	logMsg("removing device: %s,%d", d.name(), d.enumId());
	cancelKeyRepeatTimer();
	devList.remove(&d);
	indexDevices();
}

void indexDevices()
{
	// re-number device indices
	uint i = 0;
	for(auto &e : Input::devList)
	{
		e->idx = i;
		i++;
	}
}

uint Device::map() const
{
	return
		#ifdef CONFIG_INPUT_ICADE
		iCadeMode() ? (uint)Input::Event::MAP_ICADE :
		#endif
		map_;
}

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

bool swappedGamepadConfirm = SWAPPED_GAMEPAD_CONFIRM_DEFAULT;

//struct PointerState
//{
//	constexpr PointerState() { }
//	int inWin = 0;
//};

static uint xPointerTransform_ = POINTER_NORMAL;
void xPointerTransform(uint mode)
{
	xPointerTransform_ = mode;
}

static uint yPointerTransform_ = POINTER_NORMAL;
void yPointerTransform(uint mode)
{
	yPointerTransform_ = mode;
}

static uint pointerAxis_ = POINTER_NORMAL;
void pointerAxis(uint mode)
{
	pointerAxis_ = mode;
}

IG::Point2D<int> pointerPos(const Base::Window &win, int x, int y)
{
	IG::Point2D<int> pos;
	// x,y axis is swapped first
	pos.x = pointerAxis_ == POINTER_INVERT ? y : x;
	pos.y = pointerAxis_ == POINTER_INVERT ? x : y;
	
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
	xPointerTransform(win.rotateView == VIEW_ROTATE_0 || win.rotateView == VIEW_ROTATE_90 ? POINTER_NORMAL : POINTER_INVERT);
	yPointerTransform(win.rotateView == VIEW_ROTATE_0 || win.rotateView == VIEW_ROTATE_270 ? POINTER_NORMAL : POINTER_INVERT);
	pointerAxis(win.rotateView == VIEW_ROTATE_0 || win.rotateView == VIEW_ROTATE_180 ? POINTER_NORMAL : POINTER_INVERT);
}

#ifdef INPUT_SUPPORTS_KEYBOARD
static const char *keyButtonName(Key b)
{
	using namespace Keycode;
	switch(b)
	{
		case 0: return "None";

		case asciiKey(' '): return "Space";
		#if !defined CONFIG_BASE_ANDROID && !defined CONFIG_BASE_WIN32
		// no unique codes for these on Android
		case asciiKey('{'): return "{";
		case asciiKey('|'): return "|";
		case asciiKey('}'): return "}";
		case asciiKey('~'): return "~";
		case asciiKey('!'): return "!";
		case asciiKey('"'): return "\"";
		case asciiKey('$'): return "$";
		case asciiKey('%'): return "%";
		case asciiKey('&'): return "&";
		case asciiKey('('): return "(";
		case asciiKey(')'): return ")";
		case asciiKey('>'): return ">";
		case asciiKey('<'): return "<";
		case asciiKey('?'): return "?";
		case asciiKey(':'): return ":";
		case asciiKey('^'): return "^";
		case asciiKey('_'): return "_";
		case asciiKey('A'): return "A";
		case asciiKey('B'): return "B";
		case asciiKey('C'): return "C";
		case asciiKey('D'): return "D";
		case asciiKey('E'): return "E";
		case asciiKey('F'): return "F";
		case asciiKey('G'): return "G";
		case asciiKey('H'): return "H";
		case asciiKey('I'): return "I";
		case asciiKey('J'): return "J";
		case asciiKey('K'): return "K";
		case asciiKey('L'): return "L";
		case asciiKey('M'): return "M";
		case asciiKey('N'): return "N";
		case asciiKey('O'): return "O";
		case asciiKey('P'): return "P";
		case asciiKey('Q'): return "Q";
		case asciiKey('R'): return "R";
		case asciiKey('S'): return "S";
		case asciiKey('T'): return "T";
		case asciiKey('U'): return "U";
		case asciiKey('V'): return "V";
		case asciiKey('W'): return "W";
		case asciiKey('X'): return "X";
		case asciiKey('Y'): return "Y";
		case asciiKey('Z'): return "Z";
		#endif
		case asciiKey('+'): return "+";
		case asciiKey('\''): return "'";
		case asciiKey(','): return ",";
		case asciiKey('-'): return "-";
		case asciiKey('.'): return ".";
		case asciiKey('/'): return "/";
		case asciiKey('#'): return "#";
		case asciiKey('*'): return "*";
		case asciiKey('0'): return "0";
		case asciiKey('1'): return "1";
		case asciiKey('2'): return "2";
		case asciiKey('3'): return "3";
		case asciiKey('4'): return "4";
		case asciiKey('5'): return "5";
		case asciiKey('6'): return "6";
		case asciiKey('7'): return "7";
		case asciiKey('8'): return "8";
		case asciiKey('9'): return "9";
		case asciiKey(';'): return ";";
		case asciiKey('='): return "=";
		case asciiKey('@'): return "@";
		case asciiKey('['): return "[";
		case asciiKey('\\'): return "\\";
		case asciiKey(']'): return "]";
		case asciiKey('`'): return "`";
		case asciiKey('a'): return "a";
		case asciiKey('b'): return "b";
		case asciiKey('c'): return "c";
		case asciiKey('d'): return "d";
		case asciiKey('e'): return "e";
		case asciiKey('f'): return "f";
		case asciiKey('g'): return "g";
		case asciiKey('h'): return "h";
		case asciiKey('i'): return "i";
		case asciiKey('j'): return "j";
		case asciiKey('k'): return "k";
		case asciiKey('l'): return "l";
		case asciiKey('m'): return "m";
		case asciiKey('n'): return "n";
		case asciiKey('o'): return "o";
		case asciiKey('p'): return "p";
		case asciiKey('q'): return "q";
		case asciiKey('r'): return "r";
		case asciiKey('s'): return "s";
		case asciiKey('t'): return "t";
		case asciiKey('u'): return "u";
		case asciiKey('v'): return "v";
		case asciiKey('w'): return "w";
		case asciiKey('x'): return "x";
		case asciiKey('y'): return "y";
		case asciiKey('z'): return "z";
		case Keycode::ESCAPE:
		#ifdef CONFIG_BASE_ANDROID
		return "Back";
		#else
		return "Escape";
		#endif
		case Keycode::ENTER: return "Enter";
		case Keycode::LALT: return "Left Alt";
		case Keycode::RALT:
		#if defined CONFIG_ENV_WEBOS && CONFIG_ENV_WEBOS_OS <= 2
		return "Mod";
		#else
		return "Right Alt";
		#endif
		case Keycode::LSHIFT: return "Left Shift";
		case Keycode::RSHIFT: return "Right Shift";
		case Keycode::LCTRL: return "Left Ctrl";
		case Keycode::RCTRL:
		#if defined CONFIG_ENV_WEBOS && CONFIG_ENV_WEBOS_OS <= 2
		return "Sym";
		#else
		return "Right Ctrl";
		#endif
		case Keycode::UP: return "Up";
		case Keycode::RIGHT: return "Right";
		case Keycode::DOWN: return "Down";
		case Keycode::LEFT: return "Left";
		case Keycode::BACK_SPACE: return "Back Space";
		case Keycode::MENU: return "Menu";
		case Keycode::HOME: return "Home";
		case Keycode::END: return "End";
		case Keycode::INSERT: return "Insert";
		case Keycode::DELETE: return "Delete";
		case Keycode::TAB: return "Tab";
		case Keycode::SCROLL_LOCK: return "Scroll Lock";
		case Keycode::CAPS: return "Caps Lock";
		case Keycode::PAUSE: return "Pause";
		#ifdef CONFIG_BASE_X11
		case Keycode::LMETA: return "Left Meta";
		case Keycode::RMETA: return "Right Meta";
		#endif
		case Keycode::LSUPER: return "Left Start/Option";
		case Keycode::RSUPER: return "Right Start/Option";
		case Keycode::PGUP: return "Page Up";
		case Keycode::PGDOWN: return "Page Down";
		case Keycode::PRINT_SCREEN: return "Print Screen";
		case Keycode::NUM_LOCK: return "Num Lock";
		case Keycode::NUMPAD_0: return "Numpad 0";
		case Keycode::NUMPAD_1: return "Numpad 1";
		case Keycode::NUMPAD_2: return "Numpad 2";
		case Keycode::NUMPAD_3: return "Numpad 3";
		case Keycode::NUMPAD_4: return "Numpad 4";
		case Keycode::NUMPAD_5: return "Numpad 5";
		case Keycode::NUMPAD_6: return "Numpad 6";
		case Keycode::NUMPAD_7: return "Numpad 7";
		case Keycode::NUMPAD_8: return "Numpad 8";
		case Keycode::NUMPAD_9: return "Numpad 9";
		case Keycode::NUMPAD_DIV: return "Numpad /";
		case Keycode::NUMPAD_MULT: return "Numpad *";
		case Keycode::NUMPAD_SUB: return "Numpad -";
		case Keycode::NUMPAD_ADD: return "Numpad +";
		case Keycode::NUMPAD_DOT: return "Numpad .";
		case Keycode::NUMPAD_COMMA: return "Numpad ,";
		#ifndef CONFIG_BASE_WIN32
		case Keycode::NUMPAD_ENTER: return "Numpad Enter";
		case Keycode::NUMPAD_EQUALS: return "Numpad =";
		#endif
		#ifdef CONFIG_BASE_X11
		case Keycode::NUMPAD_INSERT: return "Numpad Insert";
		case Keycode::NUMPAD_DELETE: return "Numpad Delete";
		case Keycode::NUMPAD_BEGIN: return "Numpad Begin";
		case Keycode::NUMPAD_HOME: return "Numpad Home";
		case Keycode::NUMPAD_END: return "Numpad End";
		case Keycode::NUMPAD_PGUP: return "Numpad Page Up";
		case Keycode::NUMPAD_PGDOWN: return "Numpad Page Down";
		case Keycode::NUMPAD_UP: return "Numpad Up";
		case Keycode::NUMPAD_RIGHT: return "Numpad Right";
		case Keycode::NUMPAD_DOWN: return "Numpad Down";
		case Keycode::NUMPAD_LEFT: return "Numpad Left";
		#endif
		case Keycode::F1: return "F1";
		case Keycode::F2: return "F2";
		case Keycode::F3: return "F3";
		case Keycode::F4: return "F4";
		case Keycode::F5: return "F5";
		case Keycode::F6: return "F6";
		case Keycode::F7: return "F7";
		case Keycode::F8: return "F8";
		case Keycode::F9: return "F9";
		case Keycode::F10: return "F10";
		case Keycode::F11: return "F11";
		case Keycode::F12: return "F12";
		#ifndef CONFIG_ENV_WEBOS
		case Keycode::SEARCH: return "Search";
		#endif

		// Android-specific
		#ifdef CONFIG_BASE_ANDROID
		case Keycode::CENTER: return "Center";
		case Keycode::CAMERA: return "Camera";
		case Keycode::CALL: return "Call";
		case Keycode::END_CALL: return "End Call";
		case Keycode::CLEAR: return "Clear";
		case Keycode::SYMBOL: return "Sym";
		case Keycode::EXPLORER: return "Explorer";
		case Keycode::MAIL: return "Mail";
		case Keycode::NUM: return "Num";
		case Keycode::FUNCTION: return "Function";
		case Keycode::GAME_A: return "A";
		case Keycode::GAME_B: return "B";
		case Keycode::GAME_C: return "C";
		case Keycode::GAME_X: return "X";
		case Keycode::GAME_Y: return "Y";
		case Keycode::GAME_Z: return "Z";
		case Keycode::GAME_L1: return "L1";
		case Keycode::GAME_R1: return "R1";
		case Keycode::GAME_L2: return "L2";
		case Keycode::GAME_R2: return "R2";
		case Keycode::GAME_LEFT_THUMB: return "L-Thumb";
		case Keycode::GAME_RIGHT_THUMB: return "R-Thumb";
		case Keycode::GAME_START: return "Start";
		case Keycode::GAME_SELECT: return "Select";
		case Keycode::GAME_MODE: return "Mode";
		case Keycode::GAME_1: return "G1";
		case Keycode::GAME_2: return "G2";
		case Keycode::GAME_3: return "G3";
		case Keycode::GAME_4: return "G4";
		case Keycode::GAME_5: return "G5";
		case Keycode::GAME_6: return "G6";
		case Keycode::GAME_7: return "G7";
		case Keycode::GAME_8: return "G8";
		case Keycode::GAME_9: return "G9";
		case Keycode::GAME_10: return "G10";
		case Keycode::GAME_11: return "G11";
		case Keycode::GAME_12: return "G12";
		case Keycode::GAME_13: return "G13";
		case Keycode::GAME_14: return "G14";
		case Keycode::GAME_15: return "G15";
		case Keycode::GAME_16: return "G16";
		case Keycode::VOL_UP: return "Vol Up";
		case Keycode::VOL_DOWN: return "Vol Down";
		case Keycode::FOCUS: return "Focus";
		case Keycode::HEADSET_HOOK: return "Headset Hook";

		case Keycode::JS1_XAXIS_POS: return "X Axis+";
		case Keycode::JS1_XAXIS_NEG: return "X Axis-";
		case Keycode::JS1_YAXIS_POS: return "Y Axis+";
		case Keycode::JS1_YAXIS_NEG: return "Y Axis-";
		case Keycode::JS2_XAXIS_POS: return "X Axis+ 2";
		case Keycode::JS2_XAXIS_NEG: return "X Axis- 2";
		case Keycode::JS2_YAXIS_POS: return "Y Axis+ 2";
		case Keycode::JS2_YAXIS_NEG: return "Y Axis- 2";
		case Keycode::JS3_XAXIS_POS: return "X Axis+ 3";
		case Keycode::JS3_XAXIS_NEG: return "X Axis- 3";
		case Keycode::JS3_YAXIS_POS: return "Y Axis+ 3";
		case Keycode::JS3_YAXIS_NEG: return "Y Axis- 3";
		case Keycode::JS_POV_XAXIS_POS: return "POV Right";
		case Keycode::JS_POV_XAXIS_NEG: return "POV Left";
		case Keycode::JS_POV_YAXIS_POS: return "POV Down";
		case Keycode::JS_POV_YAXIS_NEG: return "POV Up";
		case Keycode::JS_RUDDER_AXIS_POS: return "Rudder Right";
		case Keycode::JS_RUDDER_AXIS_NEG: return "Rudder Left";
		case Keycode::JS_WHEEL_AXIS_POS: return "Wheel Right";
		case Keycode::JS_WHEEL_AXIS_NEG: return "Wheel Left";
		case Keycode::JS_LTRIGGER_AXIS: return "L Trigger";
		case Keycode::JS_RTRIGGER_AXIS: return "R Trigger";
		case Keycode::JS_GAS_AXIS: return "Gas";
		case Keycode::JS_BRAKE_AXIS: return "Brake";
		#endif
	}
	return "Unknown";
}
#endif

#ifdef CONFIG_BLUETOOTH

static const char *wiimoteButtonName(Key b)
{
	switch(b)
	{
		case 0: return "None";
		case Wiimote::_1: return "1";
		case Wiimote::_2: return "2";
		case Wiimote::A: return "A";
		case Wiimote::B: return "B";
		case Wiimote::PLUS: return "+";
		case Wiimote::MINUS: return "-";
		case Wiimote::HOME: return "Home";
		case Wiimote::UP: return "Up";
		case Wiimote::RIGHT: return "Right";
		case Wiimote::DOWN: return "Down";
		case Wiimote::LEFT: return "Left";
		case Wiimote::NUN_C: return "C";
		case Wiimote::NUN_Z: return "Z";
		case Wiimote::NUN_STICK_LEFT: return "N:Left";
		case Wiimote::NUN_STICK_RIGHT: return "N:Right";
		case Wiimote::NUN_STICK_UP: return "N:Up";
		case Wiimote::NUN_STICK_DOWN: return "N:Down";
	}
	return "Unknown";
}

static const char *wiiCCButtonName(Key b)
{
	switch(b)
	{
		case 0: return "None";
		case WiiCC::A: return "A";
		case WiiCC::B: return "B";
		case WiiCC::PLUS: return "+";
		case WiiCC::MINUS: return "-";
		case WiiCC::HOME: return "Home";
		case WiiCC::L: return "L";
		case WiiCC::R: return "R";
		case WiiCC::ZL: return "ZL";
		case WiiCC::ZR: return "ZR";
		case WiiCC::X: return "X";
		case WiiCC::Y: return "Y";
		case WiiCC::LH: return "LH";
		case WiiCC::RH: return "RH";
		case WiiCC::LSTICK_LEFT: return "L:Left";
		case WiiCC::LSTICK_RIGHT: return "L:Right";
		case WiiCC::LSTICK_UP: return "L:Up";
		case WiiCC::LSTICK_DOWN: return "L:Down";
		case WiiCC::RSTICK_LEFT: return "R:Left";
		case WiiCC::RSTICK_RIGHT: return "R:Right";
		case WiiCC::RSTICK_UP: return "R:Up";
		case WiiCC::RSTICK_DOWN: return "R:Down";
		case WiiCC::UP: return "Up";
		case WiiCC::RIGHT: return "Right";
		case WiiCC::DOWN: return "Down";
		case WiiCC::LEFT: return "Left";
	}
	return "Unknown";
}

static const char *icpButtonName(Key b)
{
	switch(b)
	{
		case 0: return "None";
		case iControlPad::A: return "A";
		case iControlPad::B: return "B";
		case iControlPad::X: return "X";
		case iControlPad::Y: return "Y";
		case iControlPad::L: return "L";
		case iControlPad::R: return "R";
		case iControlPad::START: return "Start";
		case iControlPad::SELECT: return "Select";
		case iControlPad::LNUB_LEFT: return "L:Left";
		case iControlPad::LNUB_RIGHT: return "L:Right";
		case iControlPad::LNUB_UP: return "L:Up";
		case iControlPad::LNUB_DOWN: return "L:Down";
		case iControlPad::RNUB_LEFT: return "R:Left";
		case iControlPad::RNUB_RIGHT: return "R:Right";
		case iControlPad::RNUB_UP: return "R:Up";
		case iControlPad::RNUB_DOWN: return "R:Down";
		case iControlPad::UP: return "Up";
		case iControlPad::RIGHT: return "Right";
		case iControlPad::DOWN: return "Down";
		case iControlPad::LEFT: return "Left";
	}
	return "Unknown";
}

static const char *zeemoteButtonName(Key b)
{
	switch(b)
	{
		case 0: return "None";
		case Zeemote::A: return "A";
		case Zeemote::B: return "B";
		case Zeemote::C: return "C";
		case Zeemote::POWER: return "Power";
		case Zeemote::UP: return "Up";
		case Zeemote::RIGHT: return "Right";
		case Zeemote::DOWN: return "Down";
		case Zeemote::LEFT: return "Left";
	}
	return "Unknown";
}

#endif

#ifdef CONFIG_INPUT_ICADE

static const char *iCadeButtonName(Key b)
{
	switch(b)
	{
		case 0: return "None";
		#ifdef CONFIG_BASE_IOS
		// Show the iControlPad buttons only on iOS
		case ICade::A: return "A (iCP A)";
		case ICade::B: return "B (iCP B)";
		case ICade::C: return "C (iCP X)";
		case ICade::D: return "D (iCP Y)";
		case ICade::E: return "E (iCP R)";
		case ICade::F: return "F (iCP L)";
		case ICade::G: return "G (iCP Start)";
		case ICade::H: return "H (iCP Select)";
		#else
		case ICade::A: return "A";
		case ICade::B: return "B";
		case ICade::C: return "C";
		case ICade::D: return "D";
		case ICade::E: return "E";
		case ICade::F: return "F";
		case ICade::G: return "G";
		case ICade::H: return "H";
		#endif
		case ICade::UP: return "Up";
		case ICade::RIGHT: return "Right";
		case ICade::DOWN: return "Down";
		case ICade::LEFT: return "Left";
	}
	return nullptr;
}

#endif

static const char *ps3SysButtonName(Key b)
{
	#if defined CONFIG_BASE_ANDROID
	switch(b)
	{
		case Keycode::PS3::CROSS: return "Cross";
		case Keycode::PS3::CIRCLE: return "Circle";
		case Keycode::PS3::SQUARE: return "Square";
		case Keycode::PS3::TRIANGLE: return "Triangle";
		case Keycode::PS3::PS: return "PS";
		case Keycode::GAME_LEFT_THUMB: return "L3";
		case Keycode::GAME_RIGHT_THUMB: return "R3";
	}
	return nullptr;
	#else
	return nullptr;
	#endif
}

#if defined CONFIG_BASE_PS3 || defined CONFIG_BLUETOOTH
static const char *ps3ButtonName(Key b)
{
	switch(b)
	{
		case 0: return "None";
		case PS3::CROSS: return "Cross";
		case PS3::CIRCLE: return "Circle";
		case PS3::SQUARE: return "Square";
		case PS3::TRIANGLE: return "Triangle";
		case PS3::L1: return "L1";
		case PS3::L2: return "L2";
		case PS3::L3: return "L3";
		case PS3::R1: return "R1";
		case PS3::R2: return "R2";
		case PS3::R3: return "R3";
		case PS3::SELECT: return "Select";
		case PS3::START: return "Start";
		case PS3::UP: return "Up";
		case PS3::RIGHT: return "Right";
		case PS3::DOWN: return "Down";
		case PS3::LEFT: return "Left";
		case PS3::PS: return "PS";
		case PS3::LSTICK_UP: return "L:Up";
		case PS3::LSTICK_RIGHT: return "L:Right";
		case PS3::LSTICK_DOWN: return "L:Down";
		case PS3::LSTICK_LEFT: return "L:Left";
		case PS3::RSTICK_UP: return "R:Up";
		case PS3::RSTICK_RIGHT: return "R:Right";
		case PS3::RSTICK_DOWN: return "R:Down";
		case PS3::RSTICK_LEFT: return "R:Left";
	}
	return "Unknown";
}
#endif

#ifdef CONFIG_BASE_ANDROID
static const char *xperiaPlayButtonName(Key b)
{
	switch(b)
	{
		case Keycode::XperiaPlay::CROSS: return "Cross";
		case Keycode::XperiaPlay::CIRCLE: return "Circle";
		case Keycode::XperiaPlay::SQUARE: return "Square";
		case Keycode::XperiaPlay::TRIANGLE: return "Triangle";
	}
	return nullptr;
}

static const char *ouyaButtonName(Key b)
{
	switch(b)
	{
		case Keycode::Ouya::O: return "O";
		case Keycode::Ouya::U: return "U";
		case Keycode::Ouya::Y: return "Y";
		case Keycode::Ouya::A: return "A";
		case Keycode::Ouya::L3: return "L3";
		case Keycode::Ouya::R3: return "R3";
		case Keycode::MENU: return "System";
	}
	return nullptr;
}
#endif

#ifdef CONFIG_MACHINE_PANDORA
static const char *openPandoraButtonName(Key b)
{
	switch(b)
	{
		case Keycode::Pandora::L: return "L";
		case Keycode::Pandora::R: return "R";
		case Keycode::Pandora::A: return "A";
		case Keycode::Pandora::B: return "B";
		case Keycode::Pandora::Y: return "Y";
		case Keycode::Pandora::X: return "X";
		case Keycode::Pandora::SELECT: return "Select";
		case Keycode::Pandora::START: return "Start";
		case Keycode::Pandora::LOGO: return "Logo";
	}
	return nullptr;
}
#endif

const char *Device::keyName(Key b) const
{
	switch(map())
	{
		#ifdef INPUT_SUPPORTS_KEYBOARD
		case Input::Event::MAP_SYSTEM:
		{
			const char *name = nullptr;
			switch(subtype())
			{
				#ifdef CONFIG_BASE_ANDROID
				bcase Device::SUBTYPE_XPERIA_PLAY: name = xperiaPlayButtonName(b);
				bcase Device::SUBTYPE_OUYA_CONTROLLER: name = ouyaButtonName(b);
				bcase Device::SUBTYPE_PS3_CONTROLLER: name = ps3SysButtonName(b);
				#endif
				#ifdef CONFIG_MACHINE_PANDORA
				bcase Device::SUBTYPE_PANDORA_HANDHELD: name = openPandoraButtonName(b);
				#endif
			}
			if(!name)
				return keyButtonName(b);
			return name;
		}
		#endif
		#ifdef CONFIG_BLUETOOTH
		case Input::Event::MAP_WIIMOTE: return wiimoteButtonName(b);
		case Event::MAP_WII_CC: return wiiCCButtonName(b);
		case Input::Event::MAP_ICONTROLPAD: return icpButtonName(b);
		case Input::Event::MAP_ZEEMOTE: return zeemoteButtonName(b);
		#endif
		#ifdef CONFIG_INPUT_ICADE
		case Input::Event::MAP_ICADE:
		{
			auto name = iCadeButtonName(b);
			if(!name)
			{
				#ifdef CONFIG_BASE_IOS
					return "Unknown";
				#else
					return keyButtonName(b); // if it's not an iCade button, fall back to regular keys
				#endif
			}
			return name;
		}
		#endif
		#if defined CONFIG_BASE_PS3 || defined CONFIG_BLUETOOTH
		case Input::Event::MAP_PS3PAD: return ps3ButtonName(b);
		#endif
		#ifdef CONFIG_INPUT_EVDEV
		case Input::Event::MAP_EVDEV: return evdevButtonName(b);
		#endif
	}
	return "Unknown";
}

bool keyInputIsPresent()
{
	return Device::anyTypeBitsPresent(Device::TYPE_BIT_KEYBOARD | Device::TYPE_BIT_GAMEPAD);
}

const char *eventActionToStr(int action)
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

void dispatchInputEvent(const Input::Event &event)
{
	onInputEvent(Base::mainWindow(), event);
}

}
