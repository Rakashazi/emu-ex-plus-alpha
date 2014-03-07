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

namespace Input
{

static bool processICadeKey(char c, uint action, const Device &dev, Base::Window &win)
{
	static const char *ON_STATES  = "wdxayhujikol";
	static const char *OFF_STATES = "eczqtrfnmpgv";

	#ifndef CONFIG_BASE_IOS
	using namespace ICade;
	static const Key keycodeMap[14] =
	{
		UP, RIGHT, DOWN, LEFT,
		A, B, C, D, E, F, G, H
	};
	#endif

	if(!c)
		return false; // ignore null character

	const char *p = strchr(ON_STATES, c);
	if(p)
	{
		//logMsg("handling iCade on-state key %c", *p);
		int index = p-ON_STATES;
		if(action == PUSHED)
		{
			#ifdef CONFIG_BASE_IOS
			Event event{0, Event::MAP_ICADE, (Key)(index+1), PUSHED, 0, 0, &dev};
			#else
			Event event{0, Event::MAP_ICADE, (Key)keycodeMap[index], PUSHED, 0, 0, &dev};
			#endif
			startKeyRepeatTimer(event);
			Base::onInputEvent(win, event);
		}
		return true;
	}
	else
	{
		p = strchr(OFF_STATES, c);
		if(p)
		{
			//logMsg("handling iCade off-state key %c", *p);
			int index = p-OFF_STATES;
			if(action == PUSHED)
			{
				cancelKeyRepeatTimer();
				#ifdef CONFIG_BASE_IOS
				Base::onInputEvent(win, Input::Event{0, Event::MAP_ICADE, (Key)(index+1), RELEASED, 0, 0, &dev});
				#else
				Base::onInputEvent(win, Input::Event{0, Event::MAP_ICADE, keycodeMap[index], RELEASED, 0, 0, &dev});
				#endif
			}
			return true;
		}
	}
	return false; // not an iCade key
}

}
