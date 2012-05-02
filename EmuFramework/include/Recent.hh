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

#include <fs/sys.hh>
#include <util/basicString.h>
#include <util/collection/DLList.hh>
#include <input/interface.h>
#include <gui/MenuItem/MenuItem.hh>
#include "EmuSystem.hh"

struct RecentGameInfo
{
	FsSys::cPath path;
	char name[256];

	bool operator ==(RecentGameInfo const& rhs) const
	{
		return string_equal(path, rhs.path);
	}

	void handleMenuSelection(TextMenuItem &, const InputEvent &e);
};

extern DLList<RecentGameInfo> recentGameList;

void recent_addGame(const char *fullPath, const char *name);

static void recent_addGame()
{
	recent_addGame(EmuSystem::fullGamePath,
		strlen(EmuSystem::fullGameName) ? EmuSystem::fullGameName : EmuSystem::gameName);
}
