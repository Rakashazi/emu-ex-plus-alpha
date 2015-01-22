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

#include <imagine/engine-globals.h>
#include <imagine/util/operators.hh>

#ifdef __OBJC__
#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#endif

#define CONFIG_BASE_SCREEN_FRAME_INTERVAL

namespace Base
{

class IOSScreen : public NotEquals<IOSScreen>
{
public:
	void *uiScreen_ = nullptr; // UIScreen in ObjC
	void *displayLink_ = nullptr; // CADisplayLink in ObjC
	bool displayLinkActive = false;

	constexpr IOSScreen() {}

	bool operator ==(IOSScreen const &rhs) const
	{
		return uiScreen_ == rhs.uiScreen_;
	}

	explicit operator bool() const
	{
		return uiScreen_;
	}

	#ifdef __OBJC__
	void init(UIScreen *screen);
	UIScreen *uiScreen() { return (__bridge UIScreen*)uiScreen_; }
	CADisplayLink *displayLink() { return (__bridge CADisplayLink*)displayLink_; }
	#endif
};

using ScreenImpl = IOSScreen;

}
