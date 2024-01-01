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

#include <imagine/input/inputDefs.hh>
#include <imagine/base/Window.hh>
#include <imagine/time/Time.hh>
#include <utility>

namespace IG::Input
{

struct AxisKeyEmu
{
	static constexpr std::pair<float, float> limit{-.5, .5}; // low/high limits
	std::pair<Key, Key> keys{};
	int8_t state{};

	struct UpdateKeys
	{
		Key released = 0, pushed = 0;
		bool updated = false;
	};

	constexpr AxisKeyEmu() = default;
	constexpr AxisKeyEmu(std::pair<Key, Key> keys):
		keys{keys} {}

	UpdateKeys update(float pos);
	bool dispatch(float pos, Map map, SteadyClockTimePoint time, const Device &dev, Window &win);
};

}
