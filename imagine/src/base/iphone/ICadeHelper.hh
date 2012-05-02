#pragma once

#include <input/interface.h>
#include <input/common/iCade.hh>

struct ICadeHelper
{
	UIView *mainView, *dummyInputView;
	uint active, cycleResponder;

	void init(UIView *mainView)
	{
		if(!dummyInputView)
		{
			logMsg("init iCade helper");
			dummyInputView = [[UIView alloc] initWithFrame:CGRectZero];
			var_selfs(mainView);
		}
	}

	void setActive(uint active)
	{
		var_selfs(active);
		logMsg("set iCade active %d", active);
		if(active)
		{
			[mainView becomeFirstResponder];
		}
		else
		{
			[mainView resignFirstResponder];
		}
	}

	uint isActive()
	{
		return active;
	}

	void didEnterBackground()
	{
		if(active)
			[mainView resignFirstResponder];
	}

	void didBecomeActive()
	{
		if(active)
			[mainView becomeFirstResponder];
	}

	void insertText(NSString *text)
	{
		using namespace Input;
		//logMsg("got text %s", [text cStringUsingEncoding: NSUTF8StringEncoding]);
		char c = [text characterAtIndex:0];

		Input::processICadeKey(c, INPUT_PUSHED);

		if (++cycleResponder > 20)
		{
			// necessary to clear a buffer that accumulates internally
			cycleResponder = 0;
			[mainView resignFirstResponder];
			[mainView becomeFirstResponder];
		}
	}
};
