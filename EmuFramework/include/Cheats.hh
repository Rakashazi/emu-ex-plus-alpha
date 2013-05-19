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

#include <util/gui/BaseMenuView.hh>
#include <EmuSystem.hh>

class BaseCheatsView : public BaseMenuView
{
protected:
	TextMenuItem edit;
	MenuItem *item[EmuCheats::MAX + 1] = {nullptr};

public:
	BaseCheatsView();
	void init(bool highlightFirst);
	virtual void loadCheatItems(MenuItem *item[], uint &items) = 0;
};

class EditCheatView : public BaseMenuView
{
protected:
	TextMenuItem name, remove;

public:
	EditCheatView(const char *name);
	void loadNameItem(const char *name, MenuItem *item[], uint &items);
	void loadRemoveItem(MenuItem *item[], uint &items);
	virtual void renamed(const char *str) = 0;
	virtual void removed() = 0;
};

class BaseEditCheatListView : public BaseMenuView
{
protected:
	MenuItem *item[EmuCheats::MAX + EmuCheats::MAX_CODE_TYPES] = {nullptr};

public:
	constexpr BaseEditCheatListView(): BaseMenuView("Edit Cheats") {}
	void init(bool highlightFirst);
	virtual void loadAddCheatItems(MenuItem *item[], uint &items) = 0;
	virtual void loadCheatItems(MenuItem *item[], uint &items) = 0;
};

void refreshCheatViews();
