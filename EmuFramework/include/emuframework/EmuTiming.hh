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

#include <emuframework/config.hh>
#include <imagine/time/Time.hh>
#include <imagine/util/used.hh>

namespace EmuEx
{

using namespace IG;

struct EmuFrameTimingInfo
{
	int advanced{};
	SteadyClockDuration duration{};
};

enum class FrameTimingStatEvent
{
	startOfFrame,
	startOfEmulation,
	waitForPresent,
	endOfFrame,
};

struct FrameTimingStats
{
	SteadyClockTimePoint startOfFrame{};
	ConditionalMember<enableFullFrameTimingStats, SteadyClockTimePoint> startOfEmulation{};
	ConditionalMember<enableFullFrameTimingStats, SteadyClockTimePoint> waitForPresent{};
	SteadyClockTimePoint endOfFrame{};
	ConditionalMember<enableFullFrameTimingStats, int> missedFrameCallbacks{};
};

class EmuTiming
{
public:
	SteadyClockDuration videoFrameDuration{};
	SteadyClockTimePoint startFrameTime{};
	int64_t lastFrame{};
	int8_t savedAdvancedFrames{};
	int8_t exactFrameDivisor{};

	EmuFrameTimingInfo advanceFrames(FrameParams);
	void setFrameDuration(SteadyClockDuration);
	void reset();
};

}
