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
	if (self)
	{
		screen_ = screen;
	}
	return self;
}

- (void)onFrame
{
	//logMsg("screen update for %p", self);
	auto &screen = *screen_;
	auto timestamp = screen.displayLink().timestamp;
	/*auto now = TimeSys::now();
	logMsg("frame time stamp %f, duration %f, now %f",
		(double)timestamp, (double)screen.displayLink().duration, (double)now);*/
	screen.frameUpdate(timestamp);
	if(!screen.frameIsPosted())
	{
		//logMsg("stopping screen updates");
		screen.unpostFrame();
		screen.prevFrameTime = 0;
	}
	else
		screen.prevFrameTime = timestamp;
}

@end

namespace Base
{

void IOSScreen::init(UIScreen *screen)
{
	uiScreen_ = (void*)CFBridgingRetain(screen);
	displayLink_ = (void*)CFBridgingRetain([screen displayLinkWithTarget:[[DisplayLinkHelper alloc] initWithScreen:(Screen*)this]
	                                       selector:@selector(onFrame)]);
	displayLink().paused = YES;
	displayLinkActive = false;
	logMsg("init screen %p", uiScreen_);
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

}
