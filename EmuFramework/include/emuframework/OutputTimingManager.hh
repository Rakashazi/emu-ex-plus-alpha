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

struct FrameRateConfig
{
	constexpr bool operator==(const FrameRateConfig&) const = default;

	FrameRate rate{};
	int refreshMultiplier{};
};

class OutputTimingManager
{
public:
	static constexpr FrameDuration autoOption{};
	static constexpr FrameDuration originalOption{-1};

	constexpr OutputTimingManager() = default;
	FrameRateConfig frameRateConfig(const EmuSystem&, std::span<const FrameRate> supportedFrameRates, FrameClockSource) const;
	static bool frameRateOptionIsValid(FrameDuration time);
	bool setFrameRateOption(VideoSystem, FrameDuration frameTime);

private:
	auto& frameRateVar(this auto&& self, VideoSystem system)
	{
	  switch(system)
	  {
	    case VideoSystem::NATIVE_NTSC: return self.frameRateNative;
	    case VideoSystem::PAL: return self.frameRatePAL;
	  }
	  __builtin_unreachable();
	}

public:
	FrameDuration frameRateOption(VideoSystem vidSys) const { return frameRateVar(vidSys); }
	auto frameRateOptionAsMenuId(VideoSystem vidSys) const { return MenuId(frameRateVar(vidSys).count() > 0 ? 1 : frameRateVar(vidSys).count()); }

private:
	FrameDuration frameRateNative{};
	FrameDuration frameRatePAL{};
};

}
