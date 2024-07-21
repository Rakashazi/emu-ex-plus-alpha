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
#include <imagine/util/algorithm.h>
#include <imagine/logger/logger.h>
#include "xlibutils.h"
#include <xcb/randr.h>
#include <cmath>
#include <format>

namespace IG
{

constexpr SystemLogger log{"X11Screen"};

XScreen::XScreen(ApplicationContext ctx, InitParams params)
{
	auto &screen = params.screen;
	xScreen = &screen;
	auto &conn = params.conn;
	xMM = screen.width_in_millimeters;
	yMM = screen.height_in_millimeters;
	if(Config::MACHINE_IS_PANDORA)
	{
		// TODO: read actual frame rate value
		frameRate_ = 60;
		frameTime_ = fromHz<SteadyClockTime>(60.);
	}
	else
	{
		frameRate_ = 60;
		frameTime_ = fromHz<SteadyClockTime>(60.);
		reliableFrameTime = false;
		xcb_randr_output_t primaryOutput{};
		auto resReply = XCB_REPLY(xcb_randr_get_screen_resources, &conn, screen.root);
		if(resReply)
		{
			auto outPrimaryReply = XCB_REPLY(xcb_randr_get_output_primary, &conn, screen.root);
			if(outPrimaryReply)
			{
				primaryOutput = outPrimaryReply->output;
			}
			else
			{
				for(auto output : std::span<xcb_randr_output_t>{xcb_randr_get_screen_resources_outputs(resReply.get()),
					size_t(xcb_randr_get_screen_resources_outputs_length(resReply.get()))})
				{
					auto outputInfoReply = XCB_REPLY(xcb_randr_get_output_info, &conn, output, resReply->timestamp);
					if(outputInfoReply && outputInfoReply->connection == XCB_RANDR_CONNECTION_CONNECTED)
					{
						primaryOutput = output;
						break;
					}
				}
			}
			auto outputInfoReply = XCB_REPLY(xcb_randr_get_output_info, &conn, primaryOutput, resReply->timestamp);
			if(outputInfoReply)
			{
				auto crtcInfoReply = XCB_REPLY(xcb_randr_get_crtc_info, &conn, outputInfoReply->crtc, outputInfoReply->timestamp);
				if(crtcInfoReply)
				{
					for(auto &modeInfo : std::span<xcb_randr_mode_info_t>{xcb_randr_get_screen_resources_modes(resReply.get()),
						(size_t)xcb_randr_get_screen_resources_modes_length(resReply.get())})
					{
						if(modeInfo.id == crtcInfoReply->mode && modeInfo.htotal && modeInfo.vtotal)
						{
							frameRate_ = float(modeInfo.dot_clock) / (modeInfo.htotal * modeInfo.vtotal);
							frameTime_ = fromSeconds<SteadyClockTime>(modeInfo.htotal * modeInfo.vtotal / double(modeInfo.dot_clock));
							reliableFrameTime = true;
							break;
						}
					}
				}
			}
		}
		assert(frameTime_.count());
	}
	log.info("screen:{} {}x{} ({}x{}mm) {}Hz", (void*)&screen,
		screen.width_in_pixels, screen.height_in_pixels, (int)xMM, (int)yMM, frameRate_);
	ctx.application().emplaceFrameTimer(frameTimer, *static_cast<Screen*>(this));
}

xcb_screen_t* XScreen::nativeObject() const
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
	return xScreen->width_in_pixels;
}

int Screen::height() const
{
	return xScreen->height_in_pixels;
}

FrameRate Screen::frameRate() const { return frameRate_; }
SteadyClockTime Screen::frameTime() const { return frameTime_; }

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
			log.warn("tried to set unsupported frame rate:{}", rate);
			return;
		}
		auto cmd = std::format("sudo /usr/pandora/scripts/op_lcdrate.sh {}", (unsigned int)rate);
		int err = system(cmd.data());
		if(err)
		{
			log.error("error:{} setting frame rate", err);
			return;
		}
		frameRate_ = rate;
		frameTime_ = fromHz<SteadyClockTime>(rate);
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
	//log.info("setting frame interval:{}", interval);
	assert(interval >= 1);
}

bool Screen::supportsFrameInterval()
{
	return false;
}

bool Screen::supportsTimestamps() const
{
	return application().supportedFrameTimerType() != SupportedFrameTimer::SIMPLE;
}

std::span<const FrameRate> Screen::supportedFrameRates() const
{
	// TODO
	return {&frameRate_, 1};
}

void Screen::setVariableFrameTime(bool useVariableTime)
{
	if(!shouldUpdateFrameTimer(frameTimer, useVariableTime))
		return;
	application().emplaceFrameTimer(frameTimer, *static_cast<Screen*>(this), useVariableTime);
}

void Screen::setFrameEventsOnThisThread()
{
	unpostFrame();
	frameTimer.setEventsOnThisThread(appContext());
}

void Screen::removeFrameEvents()
{
	unpostFrame();
}

}
