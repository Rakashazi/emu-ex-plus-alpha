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
	static const uint stateSlots = 11;
	TextMenuItem stateSlot[stateSlots];

	MenuItem *item[stateSlots];
public:
	constexpr StateSlotView(): BaseMenuView("State Slot")
	#ifdef CONFIG_CXX11
	, item CXX11_INIT_LIST({0})
	#endif
	{ }

	void init(bool highlightFirst);
	void onSelectElement(const GuiTable1D *table, const InputEvent &e, uint i);
};
