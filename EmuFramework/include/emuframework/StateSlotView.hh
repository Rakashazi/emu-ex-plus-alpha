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

#include <emuframework/EmuSystem.hh>
#include <imagine/gui/TableView.hh>

class StateSlotView : public TableView
{
private:
	static constexpr uint stateSlots = 11;
	char stateStr[stateSlots][40] {{0}};
	TextMenuItem stateSlot[stateSlots];
	MenuItem *item[stateSlots] = {nullptr};

public:
	StateSlotView(Base::Window &win): TableView{"State Slot", win} {}
	void init(bool highlightFirst);
};
