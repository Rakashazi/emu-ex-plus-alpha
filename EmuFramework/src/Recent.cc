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

#include <emuframework/Recent.hh>

StaticArrayList<RecentGameInfo, RecentGameInfo::MAX_RECENT> recentGameList;

void recent_addGame(const char *fullPath, const char *name)
{
	logMsg("adding %s to recent list, current size: %d", name, recentGameList.size());
	RecentGameInfo recent;
	string_copy(recent.path, fullPath);
	string_copy(recent.name, name);
	if(contains(recentGameList, recent)) // remove existing entry so it's added to the front
		recentGameList.remove(recent);
	else if(recentGameList.isFull()) // list full
		recentGameList.pop_back();
	recentGameList.insert(recentGameList.begin(), recent);

	/*logMsg("list contents:");
	for(auto &e : recentGameList)
	{
		logMsg("path: %s name: %s", e.path, e.name);
	}*/
}
