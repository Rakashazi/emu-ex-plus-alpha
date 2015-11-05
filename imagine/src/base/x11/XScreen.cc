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

#define LOGTAG "XScreen"
#include <imagine/base/Screen.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/algorithm.h>
#include "internal.hh"

namespace Base
{

void XScreen::init(::Screen *xScreen)
{
	assert(xScreen);
	var_selfs(xScreen);
	xMM = WidthMMOfScreen(xScreen);
	yMM = HeightMMOfScreen(xScreen);
	auto screenRes = XRRGetScreenResourcesCurrent(DisplayOfScreen(xScreen), RootWindowOfScreen(xScreen));
	auto primaryOutput = XRRGetOutputPrimary(DisplayOfScreen(xScreen), RootWindowOfScreen(xScreen));
	auto outputInfo = XRRGetOutputInfo(DisplayOfScreen(xScreen), screenRes, primaryOutput);
	auto crtcInfo = XRRGetCrtcInfo(DisplayOfScreen(xScreen), screenRes, outputInfo->crtc);
	iterateTimes(screenRes->nmode, i)
	{
		auto &modeInfo = screenRes->modes[i];
		if(modeInfo.id == crtcInfo->mode)
		{
			if(modeInfo.hTotal && modeInfo.vTotal)
			{
				frameTime_ = ((double)modeInfo.hTotal * (double)modeInfo.vTotal) / (double)modeInfo.dotClock;
			}
			else
			{
				logWarn("unknown display time");
				frameTime_ = 1. / 60.;
				reliableFrameTime = false;
			}
			break;
		}
	}
	XRRFreeCrtcInfo(crtcInfo);
	XRRFreeOutputInfo(outputInfo);
	XRRFreeScreenResources(screenRes);
	assert(frameTime_);
	logMsg("X screen: 0x%p %dx%d (%dx%dmm) %.2fHz", xScreen,
		WidthOfScreen(xScreen), HeightOfScreen(xScreen), (int)xMM, (int)yMM,
		frameTime_);
}

void Screen::deinit() {}

int Screen::width()
{
	return WidthOfScreen(xScreen);
}

int Screen::height()
{
	return HeightOfScreen(xScreen);
}

double Screen::frameRate()
{
	return 1. / frameTime_;
}

double Screen::frameTime()
{
	return frameTime_;
}

bool Screen::frameRateIsReliable()
{
	return reliableFrameTime;
}

void Screen::setFrameRate(double rate)
{
	if(Config::MACHINE_IS_PANDORA)
	{
		if(rate == DISPLAY_RATE_DEFAULT)
			rate = 60;
		rate = std::round(rate);
		if(rate != 50 && rate != 60)
		{
			logWarn("tried to set unsupported frame rate: %f", rate);
		}
		auto cmd = string_makePrintf<64>("sudo /usr/pandora/scripts/op_lcdrate.sh %u", (unsigned int)rate);
		int err = system(cmd.data());
		if(err)
		{
			logErr("error setting frame rate, %d", err);
		}
	}
}

void Screen::postFrame()
{
	if(framePosted)
		return;
	//logMsg("posting frame");
	framePosted = true;
	frameTimerScheduleVSync();
	if(!inFrameHandler)
	{
		prevFrameTimestamp = 0;
	}
}

void Screen::unpostFrame()
{
	if(!framePosted)
		return;
	//logMsg("un-posting frame");
	framePosted = false;
	frameTimerCancel();
}

void Screen::setFrameInterval(uint interval)
{
	// TODO
	//logMsg("setting frame interval %d", (int)interval);
	assert(interval >= 1);
}

bool Screen::supportsFrameInterval()
{
	return false;
}

int indexOfScreen(Screen &screen)
{
	iterateTimes(Screen::screens(), i)
	{
		if(*Screen::screen(i) == screen)
			return i;
	}
	bug_exit("screen wasn't in list");
	return 0;
}

std::vector<double> Screen::supportedFrameRates()
{
	// TODO
	std::vector<double> rateVec;
	rateVec.reserve(1);
	rateVec.emplace_back(frameRate());
	return rateVec;
}

}
