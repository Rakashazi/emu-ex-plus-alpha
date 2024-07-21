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
#include <imagine/gui/AlertView.hh>
#include <imagine/logger/logger.h>
#include <format>

namespace EmuEx
{

constexpr SystemLogger log{"StateSlotView"};

static auto slotHeadingName(EmuSystem &sys)
{
	return std::format("Set State Slot ({})", sys.stateSlot());
}

StateSlotView::StateSlotView(ViewAttachParams attach):
	TableView{"Save States", attach, menuItems},
	load
	{
		"Load State", attach,
		[this](TextMenuItem &item, View &, const Input::Event &e)
		{
			if(!item.active())
				return;
			pushAndShowModal(makeView<YesNoAlertView>("Really load state?",
				YesNoAlertView::Delegates
				{
					.onYes = [this]
					{
						if(app().loadStateWithSlot(system().stateSlot()))
							app().showEmulation();
					}
				}), e);
		}
	},
	save
	{
		"Save State", attach,
		[this](const Input::Event &e)
		{
			if(app().shouldOverwriteExistingState())
			{
				doSaveState();
			}
			else
			{
				pushAndShowModal(makeView<YesNoAlertView>("Really overwrite state?",
					YesNoAlertView::Delegates{.onYes = [this]{ doSaveState(); }}), e);
			}
		}
	},
	slotHeading{slotHeadingName(system()), attach},
	menuItems
	{
		&load, &save, &slotHeading,
		&stateSlot[0], &stateSlot[1], &stateSlot[2], &stateSlot[3], &stateSlot[4],
		&stateSlot[5], &stateSlot[6], &stateSlot[7], &stateSlot[8], &stateSlot[9]
	}
{
	assert(system().hasContent());
	refreshSlots();
}

void StateSlotView::onShow()
{
	refreshSlots();
	place();
}

void StateSlotView::refreshSlot(int slot)
{
	auto &sys = system();
	auto saveStr = sys.statePath(slot);
	auto modTimeStr = appContext().fileUriFormatLastWriteTimeLocal(saveStr);
	bool fileExists = modTimeStr.size();
	auto str = [&]()
	{
		if(fileExists)
			return std::format("{} ({})", sys.stateSlotName(slot), modTimeStr);
		else
			return std::format("{}", sys.stateSlotName(slot));
	};
	auto &s = stateSlot[slot];
	s = {str(), attachParams(), [this, slot](View&)
	{
		auto &sys = system();
		stateSlot[sys.stateSlot()].setHighlighted(false);
		stateSlot[slot].setHighlighted(true);
		sys.setStateSlot(slot);
		log.info("set state slot:{}", sys.stateSlot());
		slotHeading.compile(slotHeadingName(sys));
		load.setActive(sys.stateExists(sys.stateSlot()));
		postDraw();
	}};
	if(slot == sys.stateSlot())
		load.setActive(fileExists);
}

void StateSlotView::refreshSlots()
{
	for(auto i : iotaCount(stateSlots))
	{
		refreshSlot(i);
	}
	stateSlot[system().stateSlot()].setHighlighted(true);
}

void StateSlotView::doSaveState()
{
	auto slot = system().stateSlot();
	if(app().saveStateWithSlot(slot, false))
		app().showEmulation();
	refreshSlot(slot);
	place();
}

}
