#pragma once

namespace Input
{

namespace CONFIG_INPUT_KEYCODE_NAMESPACE
{

static constexpr uint asciiKey(uint c) { return c; }

static uint decodeAscii(Key k, bool isShiftPushed)
{
	if(isShiftPushed && (k >= 'a' || k <= 'z'))
		k -= 32;
	switch(k)
	{
		#ifdef INPUT_SUPPORTS_KEYBOARD
		case ENTER: return '\n';
		case BACK_SPACE: return '\b';
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

}
