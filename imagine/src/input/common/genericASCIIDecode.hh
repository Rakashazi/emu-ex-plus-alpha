#pragma once

namespace Input
{

static constexpr uint asciiKey(uint c) { return c; }

static uint decodeAscii(Key k, bool isShiftPushed)
{
	if(isShiftPushed && (k >= 'a' || k <= 'z'))
		k -= 32;
	switch(k)
	{
		#ifdef INPUT_SUPPORTS_KEYBOARD
		case Keycode::ENTER: return '\n';
		case Keycode::BACK_SPACE: return '\b';
		#endif
		case ' ' ... '~': return k;
	}
	return 0;
}

static bool isAsciiKey(Key k)
{
	return decodeAscii(k, 0) != 0;
}

}
