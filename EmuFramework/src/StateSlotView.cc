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

#include <emuframework/StateSlotView.hh>
#include <emuframework/EmuApp.hh>
#include "private.hh"

StateSlotView::StateSlotView(ViewAttachParams attach):
	TableView
	{
		"State Slot",
		attach,
		[this](const TableView &)
		{
			return stateSlots;
		},
		[this](const TableView &, uint idx) -> MenuItem&
		{
			return stateSlot[idx];
		}
	}
{
	for(int slot = -1; slot < 10; slot++)
	{
		auto idx = slot+1;

		if(EmuSystem::gameIsRunning())
		{
			auto saveStr = EmuSystem::sprintStateFilename(slot);
			bool fileExists = FS::exists(saveStr);
			if(fileExists)
			{
				auto mTime = FS::status(saveStr).lastWriteTimeLocal();
				char dateStr[64]{};
				std::strftime(dateStr, sizeof(dateStr), strftimeFormat, &mTime);
				string_printf(stateStr[idx], "%s (%s)", stateNameStr(slot), dateStr);
			}
			else
				string_printf(stateStr[idx], "%s", stateNameStr(slot));
			stateSlot[idx] = {stateStr[idx], {}};
			stateSlot[idx].setActive(fileExists);
		}
		else
		{
			string_printf(stateStr[idx], "%s", stateNameStr(slot));
			stateSlot[idx] = {stateStr[idx], {}};
			stateSlot[idx].setActive(false);
		}

		stateSlot[idx].setOnSelect(
			[slot](TextMenuItem &, View &view, Input::Event e)
			{
				EmuSystem::saveStateSlot = slot;
				logMsg("set state slot %d", EmuSystem::saveStateSlot);
				view.dismiss();
			});
	}
}
