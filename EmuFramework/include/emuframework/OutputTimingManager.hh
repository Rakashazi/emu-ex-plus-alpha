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

#include <emuframework/EmuSystem.hh>
#include <imagine/time/Time.hh>
#include <imagine/gui/MenuItem.hh>
#include <span>

namespace EmuEx
{

using namespace IG;

enum class FrameTimeStatEvent
{
	startOfFrame,
	startOfEmulation,
	waitForPresent,
	endOfFrame,
};

struct FrameTimeStats
{
	SteadyClockTimePoint startOfFrame{};
	SteadyClockTimePoint startOfEmulation{};
	SteadyClockTimePoint waitForPresent{};
	SteadyClockTimePoint endOfFrame{};
	int missedFrameCallbacks{};
};

struct FrameTimeConfig
{
	FrameTime time;
	FrameRate rate;
	int refreshMultiplier;
};

class OutputTimingManager
{
public:
	static constexpr FrameTime autoOption{};
	static constexpr FrameTime originalOption{-1};

	constexpr OutputTimingManager() = default;
	FrameTimeConfig frameTimeConfig(const EmuSystem &, std::span<const FrameRate> supportedFrameRates) const;
	static bool frameTimeOptionIsValid(FrameTime time);
	bool setFrameTimeOption(VideoSystem, FrameTime frameTime);

private:
	auto& frameTimeVar(this auto&& self, VideoSystem system)
	{
	  switch(system)
	  {
	    case VideoSystem::NATIVE_NTSC: return self.frameTimeNative;
	    case VideoSystem::PAL: return self.frameTimePAL;
	  }
	  __builtin_unreachable();
	}

public:
	FrameTime frameTimeOption(VideoSystem vidSys) const { return frameTimeVar(vidSys); }
	auto frameTimeOptionAsMenuId(VideoSystem vidSys) const { return MenuId(frameTimeVar(vidSys).count() > 0 ? 1 : frameTimeVar(vidSys).count()); }

private:
	FrameTime frameTimeNative{};
	FrameTime frameTimePAL{};
};

}
