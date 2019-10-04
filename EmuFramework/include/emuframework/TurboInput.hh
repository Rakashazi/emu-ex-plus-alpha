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

#include <array>
#include <imagine/logger/logger.h>

struct TurboInput
{
	struct Action
	{
		constexpr Action() {}
		uint action = 0;

		bool operator ==(uint rhs) const
		{
			return action == rhs;
		}
	};

	std::array<Action, 5> activeAction{};
	uint clock = 0;

	constexpr TurboInput() {}
	void addEvent(uint action);
	void removeEvent(uint action);
	void update();
};
