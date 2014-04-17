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

	namespace CONFIG_INPUT_KEYCODE_NAMESPACE
	{

	uint decodeAscii(Key k, bool isShiftPushed)
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

	bool isAsciiKey(Key k)
	{
		return decodeAscii(k, 0);
	}

	}

}
