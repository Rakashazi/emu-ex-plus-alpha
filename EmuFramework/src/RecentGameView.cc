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

#include <emuframework/RecentGameView.hh>
#include <emuframework/Recent.hh>
#include "private.hh"

RecentGameView::RecentGameView(ViewAttachParams attach):
	TableView
	{
		"Recent Games",
		attach,
		[this](const TableView &)
		{
			return 1 + recentGame.size();
		},
		[this](const TableView &, uint idx) -> MenuItem&
		{
			return idx < recentGame.size() ? recentGame[idx] : clear;
		}
	},
	clear
	{
		"Clear List",
		[this](Input::Event e)
		{
			dismiss();
			recentGameList.clear();
		}
	}
{
	recentGame.reserve(recentGameList.size());
	for(auto &e : recentGameList)
	{
		recentGame.emplace_back(e.name.data(),
			[&e](TextMenuItem &item, View &view, Input::Event ev)
			{
				e.handleMenuSelection(item, ev);
			});
		recentGame.back().setActive(FS::exists(e.path.data()));
	}
	clear.setActive(recentGameList.size());
}
