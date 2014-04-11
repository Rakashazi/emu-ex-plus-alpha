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

#include <imagine/input/Input.hh>
#include <imagine/base/Base.hh>
#include "../../input/common/iCade.hh"

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

		Input::processICadeKey(c, PUSHED, *devList.front(), Base::mainWindow()); // iCade device is always added first on app init

		if (++cycleResponder > 20)
		{
			// necessary to clear a buffer that accumulates internally
			cycleResponder = 0;
			[mainView resignFirstResponder];
			[mainView becomeFirstResponder];
		}
	}
};
