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
#include <emuframework/EmuSystem.hh>
#include <emuframework/EmuApp.hh>
#include "private.hh"
#include <imagine/util/format.hh>
#include <imagine/logger/logger.h>

namespace EmuEx
{

StateSlotView::StateSlotView(ViewAttachParams attach):
	TableView{"State Slot", attach, stateSlot}
{
	auto ctx = appContext();
	auto &sys = system();
	bool hasContent = sys.hasContent();
	for(auto &s : stateSlot)
	{
		int slot = std::distance(stateSlot, &s) - 1;
		if(hasContent)
		{
			auto saveStr = sys.statePath(slot);
			auto modTimeStr = ctx.fileUriFormatLastWriteTimeLocal(saveStr);
			bool fileExists = modTimeStr.size();
			auto str = [&]()
			{
				if(fileExists)
					return fmt::format("{} ({})", sys.stateSlotName(slot), modTimeStr);
				else
					return fmt::format("{}", sys.stateSlotName(slot));
			};
			s = {str(), &defaultFace(), nullptr};
			s.setActive(fileExists);
		}
		else
		{
			s = {fmt::format("{}", sys.stateSlotName(slot)), &defaultFace(), nullptr};
			s.setActive(false);
		}
		s.setOnSelect(
			[&sys, slot](View &view)
			{
				sys.setStateSlot(slot);
				logMsg("set state slot:%d", sys.stateSlot());
				view.dismiss();
			});
	}
}

}
