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
#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/Application.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/algorithm.h>
#include <imagine/util/format.hh>
#include <X11/extensions/Xrandr.h>
#include <cmath>

namespace IG
{

XScreen::XScreen(ApplicationContext ctx, InitParams params):
	frameTimer{ctx.application().makeFrameTimer(*static_cast<Screen*>(this))}
{
	auto *xScreen = (::Screen*)params.xScreen;
	assert(xScreen);
	this->xScreen = xScreen;
	xMM = WidthMMOfScreen(xScreen);
	yMM = HeightMMOfScreen(xScreen);
	if(Config::MACHINE_IS_PANDORA)
	{
		// TODO: read actual frame rate value
		frameRate_ = 60;
		frameTime_ = IG::FloatSeconds(1. / 60.);
	}
	else
	{
		auto screenRes = XRRGetScreenResourcesCurrent(DisplayOfScreen(xScreen), RootWindowOfScreen(xScreen));
		auto primaryOutput = XRRGetOutputPrimary(DisplayOfScreen(xScreen), RootWindowOfScreen(xScreen));
		if(!primaryOutput)
		{
			for(auto output : std::span<RROutput>{screenRes->outputs, size_t(screenRes->noutput)})
			{
				auto outputInfo = XRRGetOutputInfo(DisplayOfScreen(xScreen), screenRes, output);
				if(outputInfo->connection == RR_Connected)
				{
					primaryOutput = output;
					break;
				}
			}
		}
		auto outputInfo = XRRGetOutputInfo(DisplayOfScreen(xScreen), screenRes, primaryOutput);
		auto crtcInfo = XRRGetCrtcInfo(DisplayOfScreen(xScreen), screenRes, outputInfo->crtc);
		for(auto &modeInfo : std::span<XRRModeInfo>{screenRes->modes, (size_t)screenRes->nmode})
		{
			if(modeInfo.id == crtcInfo->mode)
			{
				if(modeInfo.hTotal && modeInfo.vTotal)
				{
					frameRate_ = float(modeInfo.dotClock) / (modeInfo.hTotal * modeInfo.vTotal);
					frameTime_ = FloatSeconds(modeInfo.hTotal * modeInfo.vTotal / double(modeInfo.dotClock));
				}
				else
				{
					logWarn("unknown display time");
					frameRate_ = 60;
					frameTime_ = IG::FloatSeconds(1. / 60.);
					reliableFrameTime = false;
				}
				break;
			}
		}
		XRRFreeCrtcInfo(crtcInfo);
		XRRFreeOutputInfo(outputInfo);
		XRRFreeScreenResources(screenRes);
		assert(frameTime_.count());
	}
	frameTimer.setFrameRate(frameRate_);
	logMsg("screen:%p %dx%d (%dx%dmm) %.2fHz", xScreen,
		WidthOfScreen(xScreen), HeightOfScreen(xScreen), (int)xMM, (int)yMM, frameRate_);
}

void *XScreen::nativeObject() const
{
	return xScreen;
}

std::pair<float, float> XScreen::mmSize() const
{
	return {xMM, yMM};
}

bool XScreen::operator ==(XScreen const &rhs) const
{
	return xScreen == rhs.xScreen;
}

XScreen::operator bool() const
{
	return xScreen;
}

int Screen::width() const
{
	return WidthOfScreen((::Screen*)xScreen);
}

int Screen::height() const
{
	return HeightOfScreen((::Screen*)xScreen);
}

FrameRate Screen::frameRate() const { return frameRate_; }

FloatSeconds Screen::frameTime() const { return frameTime_; }

bool Screen::frameRateIsReliable() const
{
	return reliableFrameTime;
}

void Screen::setFrameRate(FrameRate rate)
{
	if constexpr(Config::MACHINE_IS_PANDORA)
	{
		if(!rate)
			rate = 60;
		else
			rate = std::round(rate);
		if(rate != 50 && rate != 60)
		{
			logWarn("tried to set unsupported frame rate: %f", rate);
			return;
		}
		auto cmd = fmt::format("sudo /usr/pandora/scripts/op_lcdrate.sh {}", (unsigned int)rate);
		int err = system(cmd.data());
		if(err)
		{
			logErr("error setting frame rate, %d", err);
			return;
		}
		frameRate_ = rate;
		frameTime_ = FloatSeconds{1. / rate};
		frameTimer.setFrameRate(rate);
	}
	else
	{
		frameTimer.setFrameRate(rate ?: frameRate());
	}
}

void Screen::postFrameTimer()
{
	frameTimer.scheduleVSync();
}

void Screen::unpostFrameTimer()
{
	frameTimer.cancel();
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

bool Screen::supportsTimestamps() const
{
	return !std::holds_alternative<SimpleFrameTimer>(frameTimer);
}

std::span<const FrameRate> Screen::supportedFrameRates() const
{
	// TODO
	return {&frameRate_, 1};
}

}
