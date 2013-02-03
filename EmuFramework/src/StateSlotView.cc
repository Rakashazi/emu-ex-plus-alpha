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

void StateSlotView::onSelectElement(const GuiTable1D *table, const Input::Event &e, uint i)
{
	EmuSystem::saveStateSlot = i-1;
	logMsg("set state slot %d", EmuSystem::saveStateSlot);
	viewStack.popAndShow();
}

void StateSlotView::init(bool highlightFirst)
{
	uint i = 0;

	for(int slot = -1; slot < 10; slot++)
	{
		auto idx = slot+1;

		if(EmuSystem::gameIsRunning())
		{
			FsSys::cPath saveStr;
			EmuSystem::sprintStateFilename(saveStr, slot);
			bool fileExists = FsSys::fileExists(saveStr);
			if(fileExists)
			{
				FsSys::timeStr date = "";
				FsSys::mTimeAsStr(saveStr, date);
				string_printf(stateStr[idx], "%s (%s)", stateNameStr(slot), date);
			}
			else
				string_printf(stateStr[idx], "%s", stateNameStr(slot));
			stateSlot[idx].init(stateStr[idx], fileExists); item[i] = &stateSlot[idx]; i++;
		}
		else
		{
			string_printf(stateStr[idx], "%s", stateNameStr(slot));
			stateSlot[idx].init(stateStr[idx], false); item[i] = &stateSlot[idx]; i++;
		}
	}
	assert(i <= sizeofArray(item));
	BaseMenuView::init(item, i, highlightFirst);
}
