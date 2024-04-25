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
#include <imagine/fs/FS.hh>
#include <vector>

namespace EmuEx
{

using namespace IG;

class AutosaveSlotView : public TableView, public EmuAppHelper
{
public:
	class SlotTextMenuItem : public TextMenuItem
	{
	public:
		SlotTextMenuItem() = default;

		SlotTextMenuItem(std::string_view slotName, UTF16Convertible auto &&name, ViewAttachParams attach, SelectDelegate selectDel):
			TextMenuItem{IG_forward(name), attach, selectDel},
			slotName{slotName} {}

		std::string slotName;
	};

	AutosaveSlotView(ViewAttachParams attach);
	void updateItem(std::string_view name, std::string_view newName);

protected:
	TextMenuItem mainSlot;
	TextMenuItem noSaveSlot;
	TextMenuItem newSlot;
	TextMenuItem manageSlots;
	TextHeadingMenuItem actions;
	std::vector<SlotTextMenuItem> extraSlotItems;
	std::vector<MenuItem*> menuItems{};

	void refreshSlots();
	void refreshItems();
	void loadItems();
};

}
