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

namespace IG
{
class Screen;
}

namespace EmuEx
{

using namespace IG;

struct FrameTimeConfig
{
	FloatSeconds time;
	FrameRate rate;
	int refreshMultiplier;
};

class OutputTimingManager
{
public:
	static constexpr FloatSeconds autoOption{};
	static constexpr FloatSeconds originalOption{-1.};

	constexpr OutputTimingManager() = default;
	FrameTimeConfig frameTimeConfig(const EmuSystem &, const Screen &) const;
	static bool frameTimeOptionIsValid(FloatSeconds time);
	bool setFrameTimeOption(VideoSystem, FloatSeconds frameTime);
	FloatSeconds frameTimeOption(VideoSystem vidSys) const { return frameTimeVar(vidSys); }
	auto frameTimeOptionAsMenuId(VideoSystem vidSys) const { return MenuItem::Id(frameTimeVar(vidSys).count() > 0 ? 1 : frameTimeVar(vidSys).count()); }

private:
	FloatSeconds frameTimeNative{};
	FloatSeconds frameTimePAL{};

	static auto &frameTimeVar(auto &self, VideoSystem system)
	{
	  switch(system)
	  {
	    case VideoSystem::NATIVE_NTSC: return self.frameTimeNative;
	    case VideoSystem::PAL: return self.frameTimePAL;
	  }
	  __builtin_unreachable();
	}
	FloatSeconds &frameTimeVar(VideoSystem system) { return frameTimeVar(*this, system); }
	const FloatSeconds &frameTimeVar(VideoSystem system) const { return frameTimeVar(*this, system); }
};

}
