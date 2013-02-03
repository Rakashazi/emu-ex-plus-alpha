#pragma once

#include <input/Input.hh>
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
			if(active)
			{
				[mainView becomeFirstResponder];
			}
		}
	}

	void setActive(uint active)
	{
		var_selfs(active);
		logMsg("set iCade active %d", active);
		if(!mainView)
			return;
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

		Input::processICadeKey(c, PUSHED, *devList.first()); // iCade device is always added first on app init

		if (++cycleResponder > 20)
		{
			// necessary to clear a buffer that accumulates internally
			cycleResponder = 0;
			[mainView resignFirstResponder];
			[mainView becomeFirstResponder];
		}
	}
};
