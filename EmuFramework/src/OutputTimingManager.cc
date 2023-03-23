/*  This file is part of EmuFramework.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with EmuFramework.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuApp.hh>
#include <emuframework/EmuSystem.hh>
#include "EmuOptions.hh"

namespace EmuEx
{

bool OutputTimingManager::frameTimeOptionIsValid(FloatSeconds time)
{
	return time == OutputTimingManager::autoOption ||
		time == OutputTimingManager::originalOption ||
		EmuSystem::validFrameRateRange.contains(1. / time.count());
}

static FloatSeconds bestOutputTimeForScreen(const Screen &screen, FloatSeconds systemFrameTime)
{
	auto targetFrameTime = systemFrameTime;
	auto targetFrameRate = 1. / targetFrameTime.count();
	static auto selectAcceptableRate = [](double rate, double targetRate)
	{
		assumeExpr(rate > 0.);
		assumeExpr(targetRate > 0.);
		static constexpr double stretchFrameRate = 4.; // accept rates +/- this value
		do
		{
			if(std::abs(rate - targetRate) <= stretchFrameRate)
				return rate;
			rate /= 2.; // try half the rate until it falls below the target
		} while(rate > targetRate);
		return 0.;
	};;
	if(Config::envIsAndroid && screen.appContext().androidSDK() >= 30) // supports setting frame rate dynamically
	{
		double acceptableRate{};
		for(auto rate : screen.supportedFrameRates())
		{
			if(auto acceptedRate = selectAcceptableRate(rate, targetFrameRate);
				acceptedRate)
			{
				acceptableRate = acceptedRate;
				logMsg("updated rate:%.2f", acceptableRate);
			}
		}
		if(acceptableRate)
		{
			logMsg("screen's frame rate:%.2f is close system's rate:%.2f", acceptableRate, targetFrameRate);
			return FloatSeconds{1. / acceptableRate};
		}
	}
	else // check the current frame rate
	{
		auto screenRate = screen.frameRate();
		if(auto acceptedRate = selectAcceptableRate(screenRate, targetFrameRate);
			acceptedRate)
		{
			logMsg("screen's frame rate:%.2f is close system's rate:%.2f", screenRate, targetFrameRate);
			return FloatSeconds{1. / acceptedRate};
		}
	}
	return targetFrameTime;
}

bool OutputTimingManager::setFrameTimeOption(VideoSystem vidSys, FloatSeconds time)
{
	if(!frameTimeOptionIsValid(time))
		return false;
	frameTimeVar(vidSys) = time;
	return true;
}

FloatSeconds OutputTimingManager::frameTime(const EmuSystem &system, const Screen &screen) const
{
	auto t = frameTimeVar(system.videoSystem());
	assumeExpr(frameTimeOptionIsValid(t));
	if(t.count() > 0)
		return t;
	else if(t == originalOption)
		return system.frameTime();
	return bestOutputTimeForScreen(screen, system.frameTime());
}

}
