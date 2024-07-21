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

#include <emuframework/EmuTiming.hh>
#include <imagine/util/utility.h>
#include <imagine/util/math.hh>
#include <imagine/logger/logger.h>
#include <cmath>

namespace EmuEx
{

constexpr SystemLogger log{"EmuTiming"};

EmuFrameTimeInfo EmuTiming::advanceFrames(FrameParams params)
{
	auto frameTimeDiff = params.timestamp - lastFrameTimestamp_;
	auto framesDiff  = params.elapsedFrames(lastFrameTimestamp_);
	std::exchange(lastFrameTimestamp_, params.timestamp);
	if(exactFrameDivisor > 0)
	{
		savedAdvancedFrames += framesDiff;
		int elapsedFrames{};
		if(savedAdvancedFrames >= exactFrameDivisor)
		{
			auto [quot, rem] = std::div(savedAdvancedFrames, exactFrameDivisor);
			elapsedFrames = quot;
			savedAdvancedFrames = rem;
		}
		return {elapsedFrames, frameTimeDiff};
	}
	else
	{
		if(!hasTime(startFrameTime)) [[unlikely]]
		{
			// first frame
			startFrameTime = params.timestamp;
			lastFrame = 0;
			return {};
		}
		assumeExpr(timePerVideoFrame.count() > 0);
		assumeExpr(startFrameTime.time_since_epoch().count() > 0);
		assumeExpr(params.timestamp >= startFrameTime);
		auto timeTotal = params.timestamp - startFrameTime;
		auto now = divRoundClosestPositive(timeTotal.count(), timePerVideoFrame.count());
		int elapsedFrames = now - lastFrame;
		lastFrame = now;
		return {elapsedFrames, frameTimeDiff};
	}
}

void EmuTiming::setFrameTime(SteadyClockTime time)
{
	timePerVideoFrame = time;
	log.info("configured frame time:{} ({:g} fps)", timePerVideoFrame, toHz(time));
	reset();
}

void EmuTiming::reset()
{
	startFrameTime = {};
	savedAdvancedFrames = {};
}

}
