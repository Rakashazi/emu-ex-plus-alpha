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
#include <emuframework/EmuOptions.hh>
#include <imagine/logger/logger.h>

namespace EmuEx
{

constexpr SystemLogger log{"OutputTimingManager"};

bool OutputTimingManager::frameTimeOptionIsValid(FrameTime time)
{
	return time == OutputTimingManager::autoOption ||
		time == OutputTimingManager::originalOption ||
		EmuSystem::validFrameRateRange.contains(toHz(time));
}

static FrameTimeConfig bestOutputTimeForScreen(std::span<const FrameRate> supportedFrameRates, FrameTime systemFrameTime)
{
	const auto systemFrameRate = toHz(systemFrameTime);
	static auto selectAcceptableRate = [](double rate, double targetRate) -> std::pair<double, int>
	{
		assumeExpr(rate > 0);
		assumeExpr(targetRate > 0);
		int refreshMultiplier = 1;
		static constexpr double stretchFrameRate = 4.; // accept rates +/- this value
		do
		{
			log.info("considering {:g}Hz for target {:g}Hz", rate, targetRate);
			if(std::abs(rate - targetRate) <= stretchFrameRate)
				return {rate, refreshMultiplier};
			rate /= 2.; // try half the rate until it falls below the target
			refreshMultiplier++;
		} while(rate + stretchFrameRate > targetRate);
		return {};
	};
	double acceptableRate{};
	int refreshMultiplier{};
	for(auto rate : supportedFrameRates)
	{
		if(auto [acceptedRate, acceptedRefreshMultiplier] = selectAcceptableRate(rate, systemFrameRate);
			acceptedRate)
		{
			acceptableRate = acceptedRate;
			refreshMultiplier = acceptedRefreshMultiplier;
		}
	}
	if(acceptableRate)
	{
		return {fromHz<FrameTime>(acceptableRate), FrameRate(acceptableRate), refreshMultiplier};
	}
	return {systemFrameTime, FrameRate(systemFrameRate), 0};
}

bool OutputTimingManager::setFrameTimeOption(VideoSystem vidSys, FrameTime time)
{
	if(!frameTimeOptionIsValid(time))
		return false;
	frameTimeVar(vidSys) = time;
	return true;
}

FrameTimeConfig OutputTimingManager::frameTimeConfig(const EmuSystem &system, std::span<const FrameRate> supportedFrameRates) const
{
	auto t = frameTimeVar(system.videoSystem());
	assumeExpr(frameTimeOptionIsValid(t));
	if(t.count() > 0)
		return {t, FrameRate(toHz(t)), 0};
	else if(t == originalOption)
		return {system.scaledFrameTime(), FrameRate(system.scaledFrameRate()), 0};
	return bestOutputTimeForScreen(supportedFrameRates, system.scaledFrameTime());
}

}
