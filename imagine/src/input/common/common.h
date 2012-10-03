#pragma once
#include <input/interface.h>
#include <gfx/Gfx.hh>

#ifdef CONFIG_BLUETOOTH
#include <bluetooth/BluetoothInputDevScanner.hh>
#endif

struct PointerState
{
	constexpr PointerState() { }
	int x = 0, y = 0, inWin = 0;
};

bool input_swappedGamepadConfirm = 0;

static uint xPointerTransform = INPUT_POINTER_NORMAL;
void input_xPointerTransform(uint mode)
{
	xPointerTransform = mode;
}

static uint yPointerTransform = INPUT_POINTER_NORMAL;
void input_yPointerTransform(uint mode)
{
	yPointerTransform = mode;
}

static uint pointerAxis = INPUT_POINTER_NORMAL;
void input_pointerAxis(uint mode)
{
	pointerAxis = mode;
}

void pointerPos(int x, int y, int *xOut, int *yOut)
{
	// x,y axis is swapped first
	*xOut = pointerAxis == INPUT_POINTER_INVERT ? y : x;
	*yOut = pointerAxis == INPUT_POINTER_INVERT ? x : y;
	
	// then coordinates are inverted
	if(xPointerTransform == INPUT_POINTER_INVERT)
		*xOut = Gfx::viewPixelWidth() - *xOut;
	if(yPointerTransform == INPUT_POINTER_INVERT)
		*yOut = Gfx::viewPixelHeight() - *yOut;
}

namespace Input
{

//DefCallbackWithCtx(InputEventFunc, eventHandler, onInputEventHandler);

#ifdef INPUT_SUPPORTS_KEYBOARD
static const char *keyButtonName(InputButton b)
{
	switch(b)
	{
		case 0: return "None";

		case asciiKey(' '): return "Space";
		#ifndef CONFIG_BASE_ANDROID
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
		case Key::ESCAPE:
		#ifdef CONFIG_BASE_ANDROID
			return "Back";
		#else
			return "Escape";
		#endif
		case Key::ENTER: return "Enter";
		case Key::LALT: return "Left Alt";
		case Key::RALT:
		#if defined CONFIG_ENV_WEBOS && CONFIG_ENV_WEBOS_OS <= 2
			return "Mod";
		#else
			return "Right Alt";
		#endif
		case Key::LSHIFT: return "Left Shift";
		case Key::RSHIFT: return "Right Shift";
		case Key::LCTRL: return "Left Ctrl";
		case Key::RCTRL:
		#if defined CONFIG_ENV_WEBOS && CONFIG_ENV_WEBOS_OS <= 2
			return "Sym";
		#else
			return "Right Ctrl";
		#endif
		case Key::UP: return "Up";
		case Key::RIGHT: return "Right";
		case Key::DOWN: return "Down";
		case Key::LEFT: return "Left";
		case Key::BACK_SPACE: return "Back Space";
		case Key::MENU: return "Menu";
		case Key::HOME: return "Home";
		case Key::END: return "End";
		case Key::INSERT: return "Insert";
		case Key::DELETE: return "Delete";
		case Key::TAB: return "Tab";
		case Key::SCROLL_LOCK: return "Scroll Lock";
		case Key::CAPS: return "Caps Lock";
		case Key::PAUSE: return "Pause";
		case Key::LMETA: return "Left Meta";
		case Key::RMETA: return "Right Meta";
		case Key::F1: return "F1";
		case Key::F2: return "F2";
		case Key::F3: return "F3";
		case Key::F4: return "F4";
		case Key::F5: return "F5";
		case Key::F6: return "F6";
		case Key::F7: return "F7";
		case Key::F8: return "F8";
		case Key::F9: return "F9";
		case Key::F10: return "F10";
		case Key::F11: return "F11";
		case Key::F12: return "F12";
		#ifndef CONFIG_ENV_WEBOS
		case Key::SEARCH: return "Search";
		#endif

		// Android-specific
		#ifdef CONFIG_BASE_ANDROID
		case Key::CENTER: return "Center";
		case Key::CAMERA: return "Camera";
		case Key::CALL: return "Call";
		case Key::END_CALL: return "End Call";
		case Key::CLEAR: return "Clear";
		case Key::SYMBOL: return "Sym";
		case Key::EXPLORER: return "Explorer";
		case Key::MAIL: return "Mail";
		case Key::NUM: return "Num";
		case Key::FUNCTION: return "Function";
		case Key::GAME_A: return "A";
		case Key::GAME_B: return "B";
		case Key::GAME_C: return "C";
		case Key::GAME_X: return "X";
		case Key::GAME_Y: return "Y";
		case Key::GAME_Z: return "Z";
		case Key::GAME_L1: return "L1";
		case Key::GAME_R1: return "R1";
		case Key::GAME_L2: return "L2";
		case Key::GAME_R2: return "R2";
		case Key::GAME_LEFT_THUMB: return "L-Thumb";
		case Key::GAME_RIGHT_THUMB: return "R-Thumb";
		case Key::GAME_START: return "Start";
		case Key::GAME_SELECT: return "Select";
		case Key::GAME_MODE: return "Mode";
		case Key::GAME_1: return "G1";
		case Key::GAME_2: return "G2";
		case Key::GAME_3: return "G3";
		case Key::GAME_4: return "G4";
		case Key::GAME_5: return "G5";
		case Key::GAME_6: return "G6";
		case Key::GAME_7: return "G7";
		case Key::GAME_8: return "G8";
		case Key::GAME_9: return "G9";
		case Key::GAME_10: return "G10";
		case Key::GAME_11: return "G11";
		case Key::GAME_12: return "G12";
		case Key::GAME_13: return "G13";
		case Key::GAME_14: return "G14";
		case Key::GAME_15: return "G15";
		case Key::GAME_16: return "G16";
		case Key::VOL_UP: return "Vol Up";
		case Key::VOL_DOWN: return "Vol Down";
		case Key::FOCUS: return "Focus";
		#endif
	}
	return "Unknown";
}
#endif

static const char *wiimoteButtonName(InputButton b)
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
		case Wiimote::L: return "L";
		case Wiimote::R: return "R";
		case Wiimote::ZL: return "ZL";
		case Wiimote::ZR: return "ZR";
		case Wiimote::X: return "X";
		case Wiimote::Y: return "Y";
		case Wiimote::CC_LSTICK_LEFT: return "L:Left";
		case Wiimote::CC_LSTICK_RIGHT: return "L:Right";
		case Wiimote::CC_LSTICK_UP: return "L:Up";
		case Wiimote::CC_LSTICK_DOWN: return "L:Down";
		case Wiimote::CC_RSTICK_LEFT: return "R:Left";
		case Wiimote::CC_RSTICK_RIGHT: return "R:Right";
		case Wiimote::CC_RSTICK_UP: return "R:Up";
		case Wiimote::CC_RSTICK_DOWN: return "R:Down";
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

static const char *icpButtonName(InputButton b)
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

static const char *zeemoteButtonName(InputButton b)
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

static const char *iCadeButtonName(InputButton b)
{
	switch(b)
	{
		case 0: return "None";
		case ICade::A: return "A (iCP Select)";
		case ICade::B: return "B (iCP L)";
		case ICade::C: return "C (iCP Start)";
		case ICade::D: return "D (iCP R)";
		case ICade::E: return "E (iCP Y)";
		case ICade::F: return "F (iCP A)";
		case ICade::G: return "G (iCP B)";
		case ICade::H: return "H (iCP X)";
		case ICade::UP: return "Up";
		case ICade::RIGHT: return "Right";
		case ICade::DOWN: return "Down";
		case ICade::LEFT: return "Left";
	}
	return "Unknown";
}

static const char *ps3ButtonName(InputButton b)
{
	switch(b)
	{
		case 0: return "None";
		case Ps3::CROSS: return "Cross";
		case Ps3::CIRCLE: return "Circle";
		case Ps3::SQUARE: return "Square";
		case Ps3::TRIANGLE: return "Triangle";
		case Ps3::L1: return "L1";
		case Ps3::L2: return "L2";
		case Ps3::L3: return "L3";
		case Ps3::R1: return "R1";
		case Ps3::R2: return "R2";
		case Ps3::R3: return "R3";
		case Ps3::SELECT: return "Select";
		case Ps3::START: return "Start";
		case Ps3::UP: return "Up";
		case Ps3::RIGHT: return "Right";
		case Ps3::DOWN: return "Down";
		case Ps3::LEFT: return "Left";
	}
	return "Unknown";
}

const char *buttonName(uint dev, InputButton b)
{
	switch(dev)
	{
		#ifdef INPUT_SUPPORTS_KEYBOARD
		case InputEvent::DEV_KEYBOARD: return keyButtonName(b);
		#endif
		#ifdef CONFIG_BLUETOOTH
		case InputEvent::DEV_WIIMOTE: return wiimoteButtonName(b);
		case InputEvent::DEV_ICONTROLPAD: return icpButtonName(b);
		case InputEvent::DEV_ZEEMOTE: return zeemoteButtonName(b);
		#endif
		#ifdef CONFIG_INPUT_ICADE
		case InputEvent::DEV_ICADE: return iCadeButtonName(b);
		#endif
		#ifdef CONFIG_BASE_PS3
		case InputEvent::DEV_PS3PAD: return ps3ButtonName(b);
		#endif
	}
	return "Unknown";
}

bool keyInputIsPresent()
{
	return Base::isInputDevPresent(InputEvent::DEV_KEYBOARD)
	#ifdef CONFIG_BLUETOOTH
		|| Bluetooth::devsConnected()
	#endif
	#ifdef CONFIG_INPUT_ICADE
		|| Input::iCadeActive()
	#endif
	;
}

}
