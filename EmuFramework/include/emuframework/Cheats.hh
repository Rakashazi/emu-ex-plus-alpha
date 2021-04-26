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

#include <emuframework/EmuAppHelper.hh>
#include <imagine/gui/TableView.hh>
#include <imagine/gui/MenuItem.hh>
#include <vector>

using RefreshCheatsDelegate = DelegateFunc<void ()>;

class BaseCheatsView : public TableView, public EmuAppHelper<BaseCheatsView>
{
public:
	BaseCheatsView(ViewAttachParams attach);

protected:
	TextMenuItem edit{};
	std::vector<BoolMenuItem> cheat{};

	virtual void loadCheatItems() = 0;
};

class BaseEditCheatListView : public TableView, public EmuAppHelper<BaseEditCheatListView>
{
public:
	BaseEditCheatListView(ViewAttachParams attach, TableView::ItemsDelegate items, TableView::ItemDelegate item);
	void setOnCheatListChanged(RefreshCheatsDelegate del);

protected:
	std::vector<TextMenuItem> cheat{};
	RefreshCheatsDelegate onCheatListChanged_{};

	void onCheatListChanged();
	virtual void loadCheatItems() = 0;
};

class BaseEditCheatView : public TableView, public EmuAppHelper<BaseEditCheatView>
{
public:
	BaseEditCheatView(const char *name, ViewAttachParams attach, const char *cheatName,
		TableView::ItemsDelegate items, TableView::ItemDelegate item, TextMenuItem::SelectDelegate removed,
		RefreshCheatsDelegate onCheatListChanged);

protected:
	TextMenuItem name{}, remove{};
	RefreshCheatsDelegate onCheatListChanged_{};

	void onCheatListChanged();
	virtual const char *cheatNameString() const = 0;
	virtual void renamed(const char *str) = 0;
};
