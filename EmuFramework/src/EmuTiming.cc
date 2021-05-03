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

#include "EmuTiming.hh"
#include <imagine/util/utility.h>
#include <imagine/logger/logger.h>
#include <cmath>

EmuFrameTimeInfo EmuTiming::advanceFramesWithTime(IG::FrameTime time)
{
	if(!startFrameTime.count()) [[unlikely]]
	{
		// first frame
		startFrameTime = time;
		lastFrame = 0;
		return {1, std::chrono::duration_cast<IG::FrameTime>(timePerVideoFrameScaled) + startFrameTime};
	}
	assumeExpr(timePerVideoFrame.count() > 0);
	assumeExpr(startFrameTime.count() > 0);
	assumeExpr(time > startFrameTime);
	auto timeTotal = time - startFrameTime;
	uint32_t now = std::round(IG::FloatSeconds(timeTotal) / timePerVideoFrameScaled);
	auto elapsedFrames = now - lastFrame;
	lastFrame = now;
	return {elapsedFrames, std::chrono::duration_cast<IG::FrameTime>(now * timePerVideoFrameScaled) + startFrameTime};
}

void EmuTiming::setFrameTime(IG::FloatSeconds time)
{
	timePerVideoFrame = time;
	updateScaledFrameTime();
	logMsg("configured frame time:%.6f (%.2f fps)", time.count(), 1. / time.count());
	reset();
}

void EmuTiming::reset()
{
	startFrameTime = {};
}

void EmuTiming::setSpeedMultiplier(uint8_t newSpeed)
{
	if(speed == newSpeed)
		return;
	speed = newSpeed ? newSpeed : 1;
	updateScaledFrameTime();
	reset();
}

void EmuTiming::updateScaledFrameTime()
{
	if(speed > 1)
		timePerVideoFrameScaled = timePerVideoFrame / speed;
	else
		timePerVideoFrameScaled = timePerVideoFrame;
}
