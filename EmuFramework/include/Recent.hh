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

#include <fs/sys.hh>
#include <util/basicString.h>
#include <util/collection/DLList.hh>
#include <input/Input.hh>
#include <gui/MenuItem/MenuItem.hh>
#include "EmuSystem.hh"

struct RecentGameInfo
{
	constexpr RecentGameInfo() { }
	FsSys::cPath path {0};
	char name[256] {0};

	bool operator ==(RecentGameInfo const& rhs) const
	{
		return string_equal(path, rhs.path);
	}

	void handleMenuSelection(TextMenuItem &, const Input::Event &e);

	static constexpr uint MAX_RECENT = 10;
};

extern StaticDLList<RecentGameInfo, RecentGameInfo::MAX_RECENT> recentGameList;

void recent_addGame(const char *fullPath, const char *name);

static void recent_addGame()
{
	recent_addGame(EmuSystem::fullGamePath,
		strlen(EmuSystem::fullGameName) ? EmuSystem::fullGameName : EmuSystem::gameName);
}
