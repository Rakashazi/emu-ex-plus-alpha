#pragma once

namespace Input
{

namespace Key
{
	static const uint
	//SYS_HOME = 3, // Never sent to apps
	ESCAPE = 4,
	CALL = 5,
	END_CALL = 6,

	#ifdef CONFIG_BASE_ANDROID_SOFT_ORIENTATION
	// keyboard is rotated due to running internally in portrait mode
	LEFT = 19,
	RIGHT = 20,
	DOWN = 21,
	UP = 22,
	#else
	UP = 19,
	DOWN = 20,
	LEFT = 21,
	RIGHT = 22,
	#endif

	CENTER = 23,
	VOL_UP = 24,
	VOL_DOWN = 25,
	//26: power, handled by OS
	CAMERA = 27,
	CLEAR = 28,

	LALT = 57,
	RALT = 58,
	LSHIFT = 59,
	RSHIFT = 60,
	TAB = 61,

	SYMBOL = 63,
	EXPLORER = 64,
	MAIL = 65,
	ENTER = 66,
	BACK_SPACE = 67,

	NUM = 78,
	//79: head set hook
	FOCUS = 80,

	MENU = 82,
	//case 83: notification
	SEARCH = 84,
	//case 85: media play
	//case 86: media stop
	//case 87: media next
	//case 88: media prev
	//case 89: media rew
	//case 90: media ff
	//case 91: mute
	PGUP = 92,
	PGDOWN = 93,

	GAME_A = 96,
	GAME_B = 97,
	GAME_C = 98,
	GAME_X = 99,
	GAME_Y = 100,
	GAME_Z = 101,
	GAME_L1 = 102,
	GAME_R1 = 103,
	GAME_L2 = 104,
	GAME_R2 = 105,
	GAME_LEFT_THUMB = 106,
	GAME_RIGHT_THUMB = 107,
	GAME_START = 108,
	GAME_SELECT = 109,
	GAME_MODE = 110,
	//111: escape
	DELETE = 112,
	LCTRL = 113,
	RCTRL = 114,
	CAPS = 115,
	SCROLL_LOCK = 116,
	LMETA = 117,
	RMETA = 118,
	FUNCTION = 119,
	PRINT_SCREEN = 120,
	PAUSE = 121,
	HOME = 122,
	END = 123,
	INSERT = 124,

	F1 = 131, F2 = 132, F3 = 133, F4 = 134, F5 = 135, F6 = 136,
	F7 = 137, F8 = 138, F9 = 139, F10 = 140, F11 = 141, F12 = 142,

	GAME_1 = 188, GAME_2 = 189, GAME_3 = 190, GAME_4 = 191, GAME_5 = 192, GAME_6 = 193,
	GAME_7 = 194, GAME_8 = 195, GAME_9 = 196, GAME_10 = 197, GAME_11 = 198, GAME_12 = 199,
	GAME_13 = 200, GAME_14 = 201, GAME_15 = 202, GAME_16 = 203
	;

	static const uint COUNT = 0xff + 1;
}

namespace Pointer
{
	static const uint LBUTTON = 1;
	static const uint RBUTTON = 2; // TODO: add real mouse support
}

}
