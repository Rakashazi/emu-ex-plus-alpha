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

static_assert(__has_feature(objc_arc), "This file requires ARC");
#include <imagine/base/Screen.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/time/Time.hh>
#include <imagine/logger/logger.h>
#include "ios.hh"

using namespace IG;

@interface UIScreen ()
- (double)_refreshRate;
@end

@interface DisplayLinkHelper : NSObject
{
@private
	Screen *screen_;
}
@end

@implementation DisplayLinkHelper

- (id)initWithScreen:(Screen *)screen
{
	self = [super init];
	if(self)
	{
		screen_ = screen;
	}
	return self;
}

- (void)onFrame:(CADisplayLink *)displayLink
{
	auto &screen = *screen_;
	auto timestamp = fromSeconds<SteadyClockTime>(displayLink.timestamp);
	//logMsg("screen:%p, frame time stamp:%f, duration:%f",
	//	screen.uiScreen(), timestamp.count(), (double)screen.displayLink().duration);
	if(!screen.frameUpdate(SteadyClockTimePoint{timestamp}))
	{
		//logMsg("stopping screen updates");
		displayLink.paused = YES;
	}
}

@end

namespace IG
{

IOSScreen::IOSScreen(ApplicationContext, InitParams initParams)
{
	UIScreen *screen = (__bridge UIScreen*)initParams.uiScreen;
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
		if(hasAtLeastIOS8())
		{
			logMsg("has %f point scaling (%f native)", (double)[screen scale], (double)[screen nativeScale]);
		}
		for(UIScreenMode *mode in screen.availableModes)
		{
			logMsg("has mode: %dx%d", (int)mode.size.width, (int)mode.size.height);
		}
		logMsg("current mode: %dx%d", (int)screen.currentMode.size.width, (int)screen.currentMode.size.height);
		logMsg("preferred mode: %dx%d", (int)screen.preferredMode.size.width, (int)screen.preferredMode.size.height);
	}
	uiScreen_ = (void*)CFBridgingRetain(screen);
	displayLink_ = (void*)CFBridgingRetain([screen displayLinkWithTarget:[[DisplayLinkHelper alloc] initWithScreen:(Screen*)this]
	                                       selector:@selector(onFrame:)]);
	displayLink().paused = YES;
	updateDisplayLinkRunLoop();

	// note: the _refreshRate value is actually time per frame in seconds
	auto frameTime = [uiScreen() _refreshRate];
	if(!frameTime || 1. / frameTime < 20. || 1. / frameTime > 200.)
	{
		logWarn("ignoring unusual refresh rate: %f", 1. / frameTime);
		frameTime = 1. / 60.;
	}
	frameTime_ = fromSeconds<SteadyClockTime>(frameTime);
	frameRate_ = 1. / frameTime;
}

IOSScreen::~IOSScreen()
{
	logMsg("deinit screen %p", uiScreen_);
	[displayLink() invalidate];
	CFRelease(displayLink_);
	CFRelease(displayLinkRunLoop_);
	CFRelease(uiScreen_);
}

void Screen::setFrameInterval(int interval)
{
	logMsg("setting display interval %d", (int)interval);
	assert(interval >= 1);
	[displayLink() setFrameInterval:interval];
}

bool Screen::supportsFrameInterval()
{
	return true;
}

bool Screen::supportsTimestamps() const
{
	return true;
}

int Screen::width() const
{
	return uiScreen().bounds.size.width;
}

int Screen::height() const
{
	return uiScreen().bounds.size.height;
}

FrameRate Screen::frameRate() const { return frameRate_; }
SteadyClockTime Screen::frameTime() const { return frameTime_; }

bool Screen::frameRateIsReliable() const
{
	return true;
}

void Screen::postFrameTimer()
{
	displayLink().paused = NO;
}

void Screen::unpostFrameTimer()
{
	displayLink().paused = YES;
}

void Screen::setFrameRate(FrameRate)
{
	// unsupported
}

std::span<const FrameRate> Screen::supportedFrameRates() const
{
	// TODO
	return {&frameRate_, 1};
}

void Screen::setVariableFrameTime(bool)
{
	// TODO
}

void Screen::setFrameEventsOnThisThread()
{
	unpostFrame();
	removeFrameEvents();
	updateDisplayLinkRunLoop();
}

void Screen::removeFrameEvents()
{
	unpostFrame();
	if(!displayLinkRunLoop_)
		return;
	[displayLink() removeFromRunLoop:displayLinkRunLoop() forMode:NSDefaultRunLoopMode];
	CFRelease(std::exchange(displayLinkRunLoop_, nullptr));
}

void IOSScreen::updateDisplayLinkRunLoop()
{
	assert(!displayLinkRunLoop_);
	displayLinkRunLoop_ = (void*)CFBridgingRetain([NSRunLoop currentRunLoop]);
	[displayLink() addToRunLoop:displayLinkRunLoop() forMode:NSDefaultRunLoopMode];
}

}
