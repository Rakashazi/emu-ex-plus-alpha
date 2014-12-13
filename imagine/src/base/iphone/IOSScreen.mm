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

#include <imagine/base/Screen.hh>
#include <imagine/base/Base.hh>
#include <imagine/util/time/sys.hh>
#include "ios.hh"

@interface DisplayLinkHelper : NSObject
{
@private
	Base::Screen *screen_;
}
@end

@implementation DisplayLinkHelper

- (id)initWithScreen:(Base::Screen *)screen
{
	self = [super init];
	if(self)
	{
		screen_ = screen;
	}
	return self;
}

- (void)onFrame
{
	auto &screen = *screen_;
	auto timestamp = screen.displayLink().timestamp;
	//logMsg("screen: %p, frame time stamp: %f, duration: %f",
	//	screen.uiScreen(), (double)timestamp, (double)screen.displayLink().duration);*/
	if(&screen == screen.screen(0))
		screen.startDebugFrameStats(timestamp);
	screen.frameUpdate(timestamp);
	if(!screen.isPosted())
	{
		//logMsg("stopping screen updates");
		screen.unpostFrame();
		screen.prevFrameTime = 0;
	}
	else
		screen.prevFrameTime = timestamp;
	if(&screen == screen.screen(0))
		screen.endDebugFrameStats();
}

@end

namespace Base
{

void IOSScreen::init(UIScreen *screen)
{
	logMsg("init screen %p", screen);
	auto currMode = screen.currentMode;
	if(currMode.size.width == 1600 && currMode.size.height == 900)
	{
		logMsg("looking for 720p mode to improve non-native video adapter performance");
		for(UIScreenMode *mode in screen.availableModes)
		{
			if(mode.size.width == 1280 && mode.size.height == 720)
			{
				logMsg("setting 720p mode");
				screen.currentMode = mode;
				break;
			}
		}
	}
	if(Config::DEBUG_BUILD)
	{
		#ifdef CONFIG_BASE_IOS_RETINA_SCALE
		if(Base::hasAtLeastIOS8())
		{
			logMsg("has %f point scaling (%f native)", (double)[screen scale], (double)[screen nativeScale]);
		}
		#endif
		for(UIScreenMode *mode in screen.availableModes)
		{
			logMsg("has mode: %dx%d", (int)mode.size.width, (int)mode.size.height);
		}
		logMsg("current mode: %dx%d", (int)screen.currentMode.size.width, (int)screen.currentMode.size.height);
		if(!Config::MACHINE_IS_GENERIC_ARMV6)
			logMsg("preferred mode: %dx%d", (int)screen.preferredMode.size.width, (int)screen.preferredMode.size.height);
	}
	uiScreen_ = (void*)CFBridgingRetain(screen);
	displayLink_ = (void*)CFBridgingRetain([screen displayLinkWithTarget:[[DisplayLinkHelper alloc] initWithScreen:(Screen*)this]
	                                       selector:@selector(onFrame)]);
	displayLink().paused = YES;
	displayLinkActive = false;
}

void Screen::deinit()
{
	logMsg("deinit screen %p", uiScreen_);
	CFRelease(displayLink_);
	CFRelease(uiScreen_);
	*this = {};
}

void Screen::setFrameInterval(uint interval)
{
	logMsg("setting frame interval %d", (int)interval);
	assert(interval >= 1);
	[displayLink() setFrameInterval:interval];
}

bool Screen::supportsFrameInterval()
{
	return true;
}

int Screen::width()
{
	return uiScreen().bounds.size.width;
}

int Screen::height()
{
	return uiScreen().bounds.size.height;
}

uint Screen::refreshRate()
{
	return 60;
}

void Screen::postFrame()
{
	if(!appIsRunning())
	{
		logMsg("can't post screen update when app isn't running");
		return;
	}
	framePosted = true;
	if(!displayLinkActive)
	{
		displayLink().paused = NO; 
		displayLinkActive = true;
	}
}

void Screen::unpostFrame()
{
	framePosted = false;
	if(displayLinkActive)
	{
		displayLink().paused = YES;
		displayLinkActive = false;
	}
}

void Screen::setRefreshRate(uint rate)
{
	// unsupported
}

}
