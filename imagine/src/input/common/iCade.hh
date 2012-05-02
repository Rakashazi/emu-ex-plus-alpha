#pragma once

namespace Input
{

#ifndef CONFIG_BASE_IOS

static fbool iCadeActive_ = 0;
void setICadeActive(fbool active)
{
	iCadeActive_ = active;
}

fbool iCadeActive()
{
	return iCadeActive_;
}

#endif

static fbool processICadeKey(uchar c, uint action)
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
			Input::onInputEvent(InputEvent(0, InputEvent::DEV_ICADE, index+1, INPUT_PUSHED));
			//callSafe(onInputEventHandler, onInputEventHandlerCtx, InputEvent(0, InputEvent::DEV_ICADE, index+1, INPUT_PUSHED));
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
				Input::onInputEvent(InputEvent(0, InputEvent::DEV_ICADE, index+1, INPUT_RELEASED));
				//callSafe(onInputEventHandler, onInputEventHandlerCtx, InputEvent(0, InputEvent::DEV_ICADE, index+1, INPUT_RELEASED));
			return 1;
		}
	}
	return 0; // not an iCade key
}

}
