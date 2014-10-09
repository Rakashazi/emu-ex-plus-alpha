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

#include <imagine/fs/sys.hh>
#include <imagine/util/basicString.h>
#include <imagine/util/container/ArrayList.hh>
#include <imagine/input/Input.hh>
#include <imagine/gui/MenuItem.hh>
#include <emuframework/EmuSystem.hh>

struct RecentGameInfo
{
	FsSys::PathString path{};
	char name[256] {0};
	static constexpr uint MAX_RECENT = 10;

	constexpr RecentGameInfo() {}

	bool operator ==(RecentGameInfo const& rhs) const
	{
		return string_equal(path.data(), rhs.path.data());
	}

	void handleMenuSelection(TextMenuItem &, const Input::Event &e);
};

extern StaticArrayList<RecentGameInfo, RecentGameInfo::MAX_RECENT> recentGameList;

void recent_addGame(const char *fullPath, const char *name);

static void recent_addGame()
{
	recent_addGame(EmuSystem::fullGamePath(), EmuSystem::fullGameName());
}
