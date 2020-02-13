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

#include <imagine/gui/TableView.hh>
#include <imagine/gui/MenuItem.hh>
#include <vector>

using RefreshCheatsDelegate = DelegateFunc<void ()>;

class BaseCheatsView : public TableView
{
public:
	BaseCheatsView(ViewAttachParams attach);
	~BaseCheatsView() override;

protected:
	TextMenuItem edit{};
	std::vector<BoolMenuItem> cheat{};
	RefreshCheatsDelegate onRefreshCheats{};

	virtual void loadCheatItems() = 0;
};

class BaseEditCheatListView : public TableView
{
public:
	BaseEditCheatListView(ViewAttachParams attach, TableView::ItemsDelegate items, TableView::ItemDelegate item);
	~BaseEditCheatListView() override;

protected:
	std::vector<TextMenuItem> cheat{};
	RefreshCheatsDelegate onRefreshCheats{};

	virtual void loadCheatItems() = 0;
};

class BaseEditCheatView : public TableView
{
public:
	BaseEditCheatView(const char *name, ViewAttachParams attach, const char *cheatName,
		TableView::ItemsDelegate items, TableView::ItemDelegate item, TextMenuItem::SelectDelegate removed);

protected:
	TextMenuItem name{}, remove{};

	virtual void renamed(const char *str) = 0;
};
