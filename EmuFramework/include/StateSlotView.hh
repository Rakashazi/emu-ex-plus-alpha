#pragma once

/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#include <EmuSystem.hh>
#include <util/gui/BaseMenuView.hh>

class StateSlotView : public BaseMenuView
{
private:
	static constexpr uint stateSlots = 11;
	char stateStr[stateSlots][40] { { 0 } };
	TextMenuItem stateSlot[stateSlots];

	MenuItem *item[stateSlots] = {nullptr};
public:
	constexpr StateSlotView(): BaseMenuView("State Slot") { }

	void init(bool highlightFirst);
	void onSelectElement(const GuiTable1D *table, const Input::Event &e, uint i);
};
