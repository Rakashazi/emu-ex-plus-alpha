#pragma once

/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/input/Input.hh>
#include <imagine/base/Window.hh>

namespace Input
{

template <class Range>
struct AxisKeyEmu
{
	Range lowLimit = 0, highLimit = 0;
	Key lowKey = 0, highKey = 0;
	Key lowSysKey = 0, highSysKey = 0;
	int8_t state = 0;

	struct UpdateKeys
	{
		Key released = 0, sysReleased = 0,
			pushed = 0, sysPushed = 0;
		bool updated = false;
	};

	constexpr AxisKeyEmu() {}
	constexpr AxisKeyEmu(Range lowLimit, Range highLimit,
		Key lowKey, Key highKey, Key lowSysKey, Key highSysKey):
		lowLimit{lowLimit}, highLimit{highLimit},
		lowKey{lowKey}, highKey{highKey}, lowSysKey{lowSysKey}, highSysKey{highSysKey} {}

	UpdateKeys update(Range pos)
	{
		UpdateKeys keys;
		int8_t newState = (pos <= lowLimit) ? -1 :
			(pos >= highLimit) ? 1 :
			0;
		if(newState != state)
		{
			const bool stateHigh = (state > 0);
			const bool stateLow = (state < 0);
			keys.released = stateHigh ? highKey : stateLow ? lowKey : 0;
			keys.sysReleased = stateHigh ? highSysKey : stateLow ? lowSysKey : 0;
			const bool newStateHigh = (newState > 0);
			const bool newStateLow = (newState < 0);
			keys.pushed = newStateHigh ? highKey : newStateLow ? lowKey : 0;
			keys.sysPushed = newStateHigh ? highSysKey : newStateLow ? lowSysKey : 0;
			keys.updated = true;
			state = newState;
		}
		return keys;
	}

	bool dispatch(Range pos, uint32_t id, Map map, Time time, const Device &dev, Base::Window &win)
	{
		auto updateKeys = update(pos);
		auto src = Source::GAMEPAD;
		if(!updateKeys.updated)
		{
			return false; // no change
		}
		if(updateKeys.released)
		{
			win.dispatchRepeatableKeyInputEvent(Event(id, map, updateKeys.released, updateKeys.sysReleased, Action::RELEASED, 0, 0, src, time, &dev));
		}
		if(updateKeys.pushed)
		{
			Event event{id, map, updateKeys.pushed, updateKeys.sysPushed, Action::PUSHED, 0, 0, src, time, &dev};
			win.dispatchRepeatableKeyInputEvent(event);
		}
		return true;
	}
};

}
