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

#include <StateSlotView.hh>

extern ViewStack viewStack;

void StateSlotView::onSelectElement(const GuiTable1D *table, const InputEvent &e, uint i)
{
	EmuSystem::saveStateSlot = i-1;
	logMsg("set state slot %d", EmuSystem::saveStateSlot);
	viewStack.popAndShow();
}

void StateSlotView::init(bool highlightFirst)
{
	uint i = 0;
	stateSlot[0].init("Auto", EmuSystem::stateExists(-1)); item[i] = &stateSlot[i]; i++;
	stateSlot[1].init("0", EmuSystem::stateExists(0)); item[i] = &stateSlot[i]; i++;
	stateSlot[2].init("1", EmuSystem::stateExists(1)); item[i] = &stateSlot[i]; i++;
	stateSlot[3].init("2", EmuSystem::stateExists(2)); item[i] = &stateSlot[i]; i++;
	stateSlot[4].init("3", EmuSystem::stateExists(3)); item[i] = &stateSlot[i]; i++;
	stateSlot[5].init("4", EmuSystem::stateExists(4)); item[i] = &stateSlot[i]; i++;
	stateSlot[6].init("5", EmuSystem::stateExists(5)); item[i] = &stateSlot[i]; i++;
	stateSlot[7].init("6", EmuSystem::stateExists(6)); item[i] = &stateSlot[i]; i++;
	stateSlot[8].init("7", EmuSystem::stateExists(7)); item[i] = &stateSlot[i]; i++;
	stateSlot[9].init("8", EmuSystem::stateExists(8)); item[i] = &stateSlot[i]; i++;
	stateSlot[10].init("9", EmuSystem::stateExists(9)); item[i] = &stateSlot[i]; i++;
	assert(i <= sizeofArray(item));
	BaseMenuView::init(item, i, highlightFirst);
}
