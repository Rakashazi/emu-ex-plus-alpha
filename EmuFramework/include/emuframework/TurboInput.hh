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

class EmuApp;

struct TurboInput
{
	struct Action
	{
		constexpr Action() {}
		unsigned action = 0;

		bool operator ==(unsigned rhs) const
		{
			return action == rhs;
		}
	};

	std::array<Action, 5> activeAction{};
	unsigned clock = 0;

	constexpr TurboInput() {}
	void addEvent(unsigned action);
	void removeEvent(unsigned action);
	void update(EmuApp *);
};
