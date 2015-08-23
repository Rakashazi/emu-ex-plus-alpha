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

BaseCheatsView::BaseCheatsView(Base::Window &win):
	TableView{"Cheats", win},
	edit
	{
		"Add/Edit",
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			auto &editCheatListView = *makeEditCheatListView(window());
			pushAndShow(editCheatListView, e);
		}
	},
	onRefreshCheats
	{
		[this]()
		{
			auto selectedCell = selected;
			deinit();
			init();
			highlightCell(selectedCell);
			place();
		}
	}
{}

void BaseCheatsView::init()
{
	assert(!onRefreshCheatsList.isFull());
	onRefreshCheatsList.emplace_back(&onRefreshCheats);
	uint i = 0;
	edit.init(); item[i++] = &edit;
	loadCheatItems(item, i);
	assert(i <= sizeofArray(item));
	TableView::init(item, i);
}

void BaseCheatsView::deinit()
{
	TableView::deinit();
	onRefreshCheatsList.remove(&onRefreshCheats);
}

void EditCheatView::loadNameItem(const char *nameStr, MenuItem *item[], uint &items)
{
	name.init(nameStr); item[items++] = &name;
}

void EditCheatView::loadRemoveItem(MenuItem *item[], uint &items)
{
	remove.init(); item[items++] = &remove;
}

EditCheatView::EditCheatView(const char *viewName, Base::Window &win):
	TableView{viewName, win},
	name
	{
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			auto &textInputView = *new CollectTextInputView{window()};
			textInputView.init("Input description", name.t.str, getCollectTextCloseAsset());
			textInputView.onText() =
			[this](CollectTextInputView &view, const char *str)
			{
				if(str)
				{
					logMsg("setting cheat name %s", str);
					renamed(str);
					name.compile(projP);
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
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			removed();
			dismiss();
		}
	}
{}

BaseEditCheatListView::BaseEditCheatListView(Base::Window &win):
	TableView{"Edit Cheats", win},
	onRefreshCheats
	{
		[this]()
		{
			auto selectedCell = selected;
			deinit();
			init();
			highlightCell(selectedCell);
			place();
		}
	}
{}

void BaseEditCheatListView::init()
{
	assert(!onRefreshCheatsList.isFull());
	onRefreshCheatsList.emplace_back(&onRefreshCheats);
	uint i = 0;
	loadAddCheatItems(item, i);
	assert(i <= EmuCheats::MAX_CODE_TYPES);
	loadCheatItems(item, i);
	assert(i <= sizeofArray(item));
	TableView::init(item, i);
}

void BaseEditCheatListView::deinit()
{
	TableView::deinit();
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
