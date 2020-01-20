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

#define LOGTAG "Screen"
#include <imagine/base/Screen.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/algorithm.h>
#include <imagine/util/string.h>
#include "internal.hh"

namespace Base
{

void XScreen::init(::Screen *xScreen)
{
	assert(xScreen);
	this->xScreen = xScreen;
	xMM = WidthMMOfScreen(xScreen);
	yMM = HeightMMOfScreen(xScreen);
	if(Config::MACHINE_IS_PANDORA)
	{
		// TODO: read actual frame rate value
		frameTime_ = 1. / 60.;
	}
	else
	{
		auto screenRes = XRRGetScreenResourcesCurrent(DisplayOfScreen(xScreen), RootWindowOfScreen(xScreen));
		auto primaryOutput = XRRGetOutputPrimary(DisplayOfScreen(xScreen), RootWindowOfScreen(xScreen));
		if(!primaryOutput)
		{
			primaryOutput = screenRes->outputs[0];
		}
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
	}
	logMsg("screen: %p %dx%d (%dx%dmm) %.2fHz", xScreen,
		WidthOfScreen(xScreen), HeightOfScreen(xScreen), (int)xMM, (int)yMM,
		1./ frameTime_);
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

double Screen::frameRate() const
{
	return 1. / frameTime_;
}

double Screen::frameTime() const
{
	return frameTime_;
}

bool Screen::frameRateIsReliable() const
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

void Screen::setFrameInterval(int interval)
{
	// TODO
	//logMsg("setting frame interval %d", (int)interval);
	assert(interval >= 1);
}

bool Screen::supportsFrameInterval()
{
	return false;
}

bool Screen::supportsTimestamps()
{
	return true;
}

int indexOfScreen(Screen &screen)
{
	iterateTimes(Screen::screens(), i)
	{
		if(*Screen::screen(i) == screen)
			return i;
	}
	logErr("screen wasn't in list");
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
