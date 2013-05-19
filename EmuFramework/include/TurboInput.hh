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

#include <util/memory/search.h>

struct TurboInput
{
	struct Action
	{
		constexpr Action() { }
		uint action = 0;

		bool operator ==(uint rhs) const
		{
			return action == rhs;
		}
	};

	constexpr TurboInput() { }

	Action activeAction[5];

	void init()
	{
		mem_zero(activeAction);
	}

	void addEvent(uint action)
	{
		Action *slot = IG::mem_findFirstZeroValue(activeAction);
		if(slot)
		{
			slot->action = action;
			logMsg("added turbo event action %d", action);
		}
	}

	void removeEvent(uint action)
	{
		forEachInArray(activeAction, e)
		{
			if(e->action == action)
			{
				e->action = 0;
				logMsg("removed turbo event action %d", action);
			}
		}
	}
};
