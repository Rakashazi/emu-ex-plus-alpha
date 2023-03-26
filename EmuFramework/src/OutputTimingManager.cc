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

static OutputTimingManager::FrameTimeConfig bestOutputTimeForScreen(const Screen &screen, FloatSeconds systemFrameTime)
{
	auto targetFrameTime = systemFrameTime;
	auto targetFrameRate = 1. / targetFrameTime.count();
	static auto selectAcceptableRate = [](FrameRate rate, FrameRate targetRate) -> std::pair<FrameRate, int>
	{
		assumeExpr(rate > 0);
		assumeExpr(targetRate > 0);
		int refreshMultiplier = 1;
		static constexpr FrameRate stretchFrameRate = 4.; // accept rates +/- this value
		do
		{
			logMsg("considering %.2fHz for target %.2fHz", rate, targetRate);
			if(std::abs(rate - targetRate) <= stretchFrameRate)
				return {rate, refreshMultiplier};
			rate /= FrameRate{2.}; // try half the rate until it falls below the target
			refreshMultiplier++;
		} while(rate + stretchFrameRate > targetRate);
		return {};
	};;
	if(Config::envIsAndroid && screen.appContext().androidSDK() >= 30) // supports setting frame rate dynamically
	{
		FrameRate acceptableRate{};
		int refreshMultiplier{};
		for(auto rate : screen.supportedFrameRates())
		{
			if(auto [acceptedRate, acceptedRefreshMultiplier] = selectAcceptableRate(rate, targetFrameRate);
				acceptedRate)
			{
				acceptableRate = acceptedRate;
				refreshMultiplier = acceptedRefreshMultiplier;
			}
		}
		if(acceptableRate)
		{
			return {FloatSeconds{1. / acceptableRate}, acceptableRate, refreshMultiplier};
		}
	}
	else // check the current frame rate
	{
		auto screenRate = screen.frameRate();
		if(auto [acceptedRate, refreshMultiplier] = selectAcceptableRate(screenRate, targetFrameRate);
			acceptedRate)
		{
			return {FloatSeconds{1. / acceptedRate}, acceptedRate, refreshMultiplier};
		}
	}
	return {targetFrameTime, FrameRate(targetFrameRate), 1};
}

bool OutputTimingManager::setFrameTimeOption(VideoSystem vidSys, FloatSeconds time)
{
	if(!frameTimeOptionIsValid(time))
		return false;
	frameTimeVar(vidSys) = time;
	return true;
}

OutputTimingManager::FrameTimeConfig OutputTimingManager::frameTimeConfig(const EmuSystem &system, const Screen &screen) const
{
	auto t = frameTimeVar(system.videoSystem());
	assumeExpr(frameTimeOptionIsValid(t));
	if(t.count() > 0)
		return {t, FrameRate(1. / t.count()), 1};
	else if(t == originalOption)
		return {system.frameTime(), FrameRate(system.frameRate()), 1};
	return bestOutputTimeForScreen(screen, system.frameTime());
}

}
