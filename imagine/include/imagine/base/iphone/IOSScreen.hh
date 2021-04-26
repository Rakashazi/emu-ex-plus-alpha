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

#include <imagine/config/defs.hh>
#include <imagine/time/Time.hh>
#include <imagine/base/baseDefs.hh>
#include <compare>

#ifdef __OBJC__
#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#endif

namespace Base
{

class ApplicationContext;

class IOSScreen
{
public:
	struct InitParams
	{
		void *uiScreen;
	};

	constexpr IOSScreen() {}
	IOSScreen(ApplicationContext, InitParams);
	~IOSScreen();

	constexpr bool operator ==(IOSScreen const &rhs) const
	{
		return uiScreen_ == rhs.uiScreen_;
	}

	constexpr bool operator ==(ScreenId uiScreen) const
	{
		return uiScreen_ == uiScreen;
	}

	explicit operator bool() const
	{
		return uiScreen_;
	}

	#ifdef __OBJC__
	IOSScreen(UIScreen *screen);
	UIScreen *uiScreen() const { return (__bridge UIScreen*)uiScreen_; }
	CADisplayLink *displayLink() const { return (__bridge CADisplayLink*)displayLink_; }
	#endif

protected:
	void *uiScreen_{}; // UIScreen in ObjC
	void *displayLink_{}; // CADisplayLink in ObjC
	IG::FloatSeconds frameTime_{};
	bool displayLinkActive{};
};

using ScreenImpl = IOSScreen;

}
