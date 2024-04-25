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

namespace EmuEx
{

using namespace IG;

class StateSlotView : public TableView, public EmuAppHelper
{
public:
	StateSlotView(ViewAttachParams attach);
	void onShow() final;

private:
	static constexpr int stateSlots = 10;
	TextMenuItem load;
	TextMenuItem save;
	TextHeadingMenuItem slotHeading;
	TextMenuItem stateSlot[stateSlots];
	std::array<MenuItem*, 13> menuItems;

	void refreshSlot(int slot);
	void refreshSlots();
	void doSaveState();
};

}
