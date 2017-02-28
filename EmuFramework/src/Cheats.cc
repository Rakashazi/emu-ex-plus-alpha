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

#include <emuframework/Cheats.hh>
#include <emuframework/EmuApp.hh>
#include <imagine/gui/TextEntry.hh>

static StaticArrayList<RefreshCheatsDelegate*, 2> onRefreshCheatsList;

BaseCheatsView::BaseCheatsView(ViewAttachParams attach):
	TableView
	{
		"Cheats",
		attach,
		[this](const TableView &)
		{
			return 1 + cheat.size();
		},
		[this](const TableView &, uint idx) -> MenuItem&
		{
			if(idx == 0)
				return edit;
			else
				return cheat[idx - 1];
		}
	},
	edit
	{
		"Add/Edit",
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			auto &editCheatListView = *EmuSystem::makeView(attachParams(), EmuSystem::ViewID::EDIT_CHEATS);
			pushAndShow(editCheatListView, e);
		}
	},
	onRefreshCheats
	{
		[this]()
		{
			auto selectedCell = selected;
			loadCheatItems();
			highlightCell(selectedCell);
			place();
		}
	}
{
	assert(!onRefreshCheatsList.isFull());
	onRefreshCheatsList.emplace_back(&onRefreshCheats);
}

BaseCheatsView::~BaseCheatsView()
{
	onRefreshCheatsList.remove(&onRefreshCheats);
}

BaseEditCheatView::BaseEditCheatView(const char *viewName, ViewAttachParams attach, const char *cheatName,
	TableView::ItemsDelegate items, TableView::ItemDelegate item, TextMenuItem::SelectDelegate removed):
	TableView
	{
		viewName,
		attach,
		items,
		item
	},
	name
	{
		cheatName,
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			auto &textInputView = *new CollectTextInputView{attachParams()};
			textInputView.init("Input description", name.t.str, getCollectTextCloseAsset(renderer()));
			textInputView.onText() =
			[this](CollectTextInputView &view, const char *str)
			{
				if(str)
				{
					logMsg("setting cheat name %s", str);
					renamed(str);
					name.compile(renderer(), projP);
					window().postDraw();
				}
				view.dismiss();
				return 0;
			};
			modalViewController.pushAndShow(textInputView, e);
		}
	},
	remove
	{
		"Delete Cheat",
		removed
	}
{}

BaseEditCheatListView::BaseEditCheatListView(ViewAttachParams attach, TableView::ItemsDelegate items, TableView::ItemDelegate item):
	TableView
	{
		"Edit Cheats",
		attach,
		items,
		item
	},
	onRefreshCheats
	{
		[this]()
		{
			auto selectedCell = selected;
			loadCheatItems();
			highlightCell(selectedCell);
			place();
		}
	}
{
	assert(!onRefreshCheatsList.isFull());
	onRefreshCheatsList.emplace_back(&onRefreshCheats);
}

BaseEditCheatListView::~BaseEditCheatListView()
{
	onRefreshCheatsList.remove(&onRefreshCheats);
}

void refreshCheatViews()
{
	logMsg("calling refresh cheat delegates");
	auto list = onRefreshCheatsList;
	for(auto e : list)
	{
		(*e)();
	}
}
