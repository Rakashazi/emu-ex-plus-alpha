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

#include <Cheats.hh>
#include <MsgPopup.hh>
#include <TextEntry.hh>
#include <util/gui/ViewStack.hh>
#include <main/EmuCheatViews.hh>

extern MsgPopup popup;
extern ViewStack viewStack;
static StaticArrayList<RefreshCheatsDelegate*, 2> onRefreshCheatsList;

BaseCheatsView::BaseCheatsView(Base::Window &win):
	BaseMenuView("Cheats", win),
	edit
	{
		"Add/Edit",
		[this](TextMenuItem &item, const Input::Event &e)
		{
			auto &editCheatListView = *menuAllocator.allocNew<EditCheatListView>(window());
			editCheatListView.init(!e.isPointer());
			viewStack.pushAndShow(&editCheatListView, &menuAllocator);
		}
	},
	onRefreshCheats
	{
		[this]()
		{
			deinit();
			init(0);
			place();
		}
	}
{}

void BaseCheatsView::init(bool highlightFirst)
{
	assert(!onRefreshCheatsList.isFull());
	onRefreshCheatsList.emplace_back(&onRefreshCheats);
	uint i = 0;
	edit.init(); item[i++] = &edit;
	loadCheatItems(item, i);
	assert(i <= sizeofArray(item));
	BaseMenuView::init(item, i, highlightFirst);
}

void BaseCheatsView::deinit()
{
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

EditCheatView::EditCheatView(const char *viewName, Base::Window &win): BaseMenuView(viewName, win),
	name
	{
		[this](TextMenuItem &item, const Input::Event &e)
		{
			auto &textInputView = *allocModalView<CollectTextInputView>(window());
			textInputView.init("Input description", name.t.str);
			textInputView.onText() =
			[this](const char *str)
			{
				if(str)
				{
					logMsg("setting cheat name %s", str);
					renamed(str);
					name.compile();
					window().displayNeedsUpdate();
				}
				removeModalView();
				return 0;
			};
			View::addModalView(textInputView);
		}
	},
	remove
	{
		"Delete Cheat",
		[this](TextMenuItem &item, const Input::Event &e)
		{
			removed();
			viewStack.popAndShow();
		}
	}
{}

BaseEditCheatListView::BaseEditCheatListView(Base::Window &win):
	BaseMenuView("Edit Cheats", win),
	onRefreshCheats
	{
		[this]()
		{
			deinit();
			init(0);
			place();
		}
	}
{}

void BaseEditCheatListView::init(bool highlightFirst)
{
	assert(!onRefreshCheatsList.isFull());
	onRefreshCheatsList.emplace_back(&onRefreshCheats);
	uint i = 0;
	loadAddCheatItems(item, i);
	assert(i == EmuCheats::MAX_CODE_TYPES);
	loadCheatItems(item, i);
	assert(i <= sizeofArray(item));
	BaseMenuView::init(item, i, highlightFirst);
}

void BaseEditCheatListView::deinit()
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
