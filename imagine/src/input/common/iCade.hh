#pragma once

namespace Input
{

static bool processICadeKey(uchar c, uint action, const Device &dev)
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

	if(c == 0)
		return 0; // ignore null character

	const char *p = strchr(ON_STATES, c);
	if(p)
	{
		//logMsg("handling iCade on-state key %c", *p);
		int index = p-ON_STATES;
		if(action == PUSHED)
			#ifdef CONFIG_BASE_IOS
				onInputEvent(Input::Event(0, Event::MAP_ICADE, index+1, PUSHED, 0, &dev));
			#else
				onInputEvent(Input::Event(0, Event::MAP_ICADE, keycodeMap[index], PUSHED, 0, &dev));
			#endif
		return 1;
	}
	else
	{
		p = strchr(OFF_STATES, c);
		if(p)
		{
			//logMsg("handling iCade off-state key %c", *p);
			int index = p-OFF_STATES;
			if(action == PUSHED)
				#ifdef CONFIG_BASE_IOS
					onInputEvent(Input::Event(0, Event::MAP_ICADE, index+1, RELEASED, 0, &dev));
				#else
					onInputEvent(Input::Event(0, Event::MAP_ICADE, keycodeMap[index], RELEASED, 0, &dev));
				#endif
			return 1;
		}
	}
	return 0; // not an iCade key
}

}
