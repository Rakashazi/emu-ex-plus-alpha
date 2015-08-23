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

void StateSlotView::init()
{
	uint i = 0;

	for(int slot = -1; slot < 10; slot++)
	{
		auto idx = slot+1;

		if(EmuSystem::gameIsRunning())
		{
			auto saveStr = EmuSystem::sprintStateFilename(slot);
			bool fileExists = FS::exists(saveStr);
			if(fileExists)
			{
				auto mTime = FS::status(saveStr).last_write_time_local();
				char dateStr[64]{};
				std::strftime(dateStr, sizeof(dateStr), strftimeFormat, &mTime);
				string_printf(stateStr[idx], "%s (%s)", stateNameStr(slot), dateStr);
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

		stateSlot[idx].onSelect() =
			[slot](TextMenuItem &, View &view, Input::Event e)
			{
				EmuSystem::saveStateSlot = slot;
				logMsg("set state slot %d", EmuSystem::saveStateSlot);
				view.dismiss();
			};
	}
	assert(i <= sizeofArray(item));
	TableView::init(item, i);
}
