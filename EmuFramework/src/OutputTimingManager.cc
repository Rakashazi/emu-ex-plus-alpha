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

#include <emuframework/OutputTimingManager.hh>
#include <emuframework/EmuSystem.hh>
#include <emuframework/EmuOptions.hh>
#include <imagine/logger/logger.h>

namespace EmuEx
{

constexpr SystemLogger log{"OutputTimingManager"};

bool OutputTimingManager::frameRateOptionIsValid(FrameDuration d)
{
	return d == OutputTimingManager::autoOption ||
		d == OutputTimingManager::originalOption ||
		EmuSystem::validFrameRateRange.contains(toHz(d));
}

static FrameRateConfig bestOutputRateForScreen(std::span<const FrameRate> supportedFrameRates, FrameRate systemFrameRate)
{
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
			refreshMultiplier *= 2;
		} while(rate + stretchFrameRate > targetRate);
		return {};
	};
	double acceptableRate{};
	int refreshMultiplier{};
	for(auto rate : supportedFrameRates)
	{
		if(auto [acceptedRate, acceptedRefreshMultiplier] = selectAcceptableRate(rate.hz(), systemFrameRate.hz());
			acceptedRate)
		{
			acceptableRate = acceptedRate;
			refreshMultiplier = acceptedRefreshMultiplier;
		}
	}
	if(acceptableRate)
	{
		return {acceptableRate, refreshMultiplier};
	}
	return {systemFrameRate, 0};
}

bool OutputTimingManager::setFrameRateOption(VideoSystem vidSys, FrameDuration d)
{
	if(!frameRateOptionIsValid(d))
		return false;
	frameRateVar(vidSys) = d;
	return true;
}

FrameRateConfig OutputTimingManager::frameRateConfig(const EmuSystem &system, std::span<const FrameRate> supportedFrameRates, FrameClockSource frameClockSrc) const
{
	auto t = frameRateVar(system.videoSystem());
	assumeExpr(frameRateOptionIsValid(t));
	if(t.count() > 0)
		return {t, 0};
	else if(t == originalOption || frameClockSrc == FrameClockSource::Timer)
		return {system.scaledFrameRate(), 0};
	return bestOutputRateForScreen(supportedFrameRates, system.scaledFrameRate());
}

}
