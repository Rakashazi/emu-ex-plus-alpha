#pragma once

namespace Input
{

#ifndef CONFIG_BASE_IOS

static bool iCadeActive_ = 0;
void setICadeActive(bool active)
{
	iCadeActive_ = active;
}

bool iCadeActive()
{
	return iCadeActive_;
}

#endif

static bool processICadeKey(uchar c, uint action)
{
	static const char *ON_STATES  = "wdxayhujikol";
	static const char *OFF_STATES = "eczqtrfnmpgv";

	if(c == 0)
		return 0; // ignore null character

	const char *p = strchr(ON_STATES, c);
	if(p)
	{
		//logMsg("handling iCade on-state key %c", *p);
		int index = p-ON_STATES;
		if(action == INPUT_PUSHED)
			Input::onInputEvent(InputEvent(0, InputEvent::DEV_ICADE, index+1, INPUT_PUSHED, 0));
		return 1;
	}
	else
	{
		p = strchr(OFF_STATES, c);
		if(p)
		{
			//logMsg("handling iCade off-state key %c", *p);
			int index = p-OFF_STATES;
			if(action == INPUT_PUSHED)
				Input::onInputEvent(InputEvent(0, InputEvent::DEV_ICADE, index+1, INPUT_RELEASED, 0));
			return 1;
		}
	}
	return 0; // not an iCade key
}

}
