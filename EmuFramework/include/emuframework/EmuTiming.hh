#pragma once

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

#include <imagine/time/Time.hh>

namespace EmuEx
{

using namespace IG;

struct EmuFrameTimeInfo
{
	int advanced{};
	SteadyClockTime frameTimeDiff{};
};

class EmuTiming
{
public:
	EmuFrameTimeInfo advanceFrames(FrameParams);
	void setFrameTime(SteadyClockTime time);
	void reset();
	SteadyClockTimePoint lastFrameTimestamp() const { return lastFrameTimestamp_; }

protected:
	SteadyClockTime timePerVideoFrame{};
	SteadyClockTimePoint startFrameTime{};
	SteadyClockTimePoint lastFrameTimestamp_{};
	int64_t lastFrame{};
	int8_t savedAdvancedFrames{};
public:
	int8_t exactFrameDivisor{};
};

}
