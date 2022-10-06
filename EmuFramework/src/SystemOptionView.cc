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

#include <emuframework/SystemOptionView.hh>
#include <emuframework/EmuApp.hh>
#include "EmuOptions.hh"
#include <imagine/base/ApplicationContext.hh>
#include <imagine/gui/TextTableView.hh>
#include <imagine/fs/FS.hh>
#include <imagine/util/format.hh>

namespace EmuEx
{

TextMenuItem::SelectDelegate SystemOptionView::setAutosaveTimerDel()
{
	return [this](TextMenuItem &item)
	{
		app().autosaveTimerMinsOption() = item.id();
		logMsg("set auto-savestate:%u", item.id());
	};
}

TextMenuItem::SelectDelegate SystemOptionView::setFastSlowModeSpeedDel()
{
	return [this](TextMenuItem &item) { app().fastSlowModeSpeedOption() = item.id(); };
}

SystemOptionView::SystemOptionView(ViewAttachParams attach, bool customMenu):
	TableView{"System Options", attach, item},
	autosaveTimerItem
	{
		{"Only On Exit",    &defaultFace(), setAutosaveTimerDel(), 0},
		{"5mins & On Exit",  &defaultFace(), setAutosaveTimerDel(), 5},
		{"10mins & On Exit", &defaultFace(), setAutosaveTimerDel(), 10},
		{"15mins & On Exit", &defaultFace(), setAutosaveTimerDel(), 15},
	},
	autosaveTimer
	{
		"Autosave Timer", &defaultFace(),
		(MenuItem::Id)app().autosaveTimerMinsOption().val,
		autosaveTimerItem
	},
	autosaveInitialSlot
	{
		"Initial Autosave Slot", &defaultFace(),
		(bool)app().confirmAutosaveSlotOption(),
		"Main", "Ask",
		[this](BoolMenuItem &item)
		{
			app().confirmAutosaveSlotOption() = item.flipBoolValue(*this);
		}
	},
	confirmOverwriteState
	{
		"Confirm Overwrite State", &defaultFace(),
		(bool)app().confirmOverwriteStateOption(),
		[this](BoolMenuItem &item)
		{
			app().confirmOverwriteStateOption() = item.flipBoolValue(*this);
		}
	},
	fastSlowModeSpeedItem
	{
		{"0.25x", &defaultFace(), setFastSlowModeSpeedDel(), 25},
		{"0.50x", &defaultFace(), setFastSlowModeSpeedDel(), 50},
		{"1.5x",  &defaultFace(), setFastSlowModeSpeedDel(), 150},
		{"2x",    &defaultFace(), setFastSlowModeSpeedDel(), 200},
		{"4x",    &defaultFace(), setFastSlowModeSpeedDel(), 400},
		{"8x",    &defaultFace(), setFastSlowModeSpeedDel(), 800},
		{"16x",   &defaultFace(), setFastSlowModeSpeedDel(), 1600},
		{"Custom Value", &defaultFace(),
			[this](const Input::Event &e)
			{
				app().pushAndShowNewCollectValueInputView<double>(attachParams(), e, "Input 0.05 to 20.0", "",
					[this](EmuApp &app, auto val)
					{
						if(val >= MIN_RUN_SPEED && val <= MAX_RUN_SPEED)
						{
							auto valAsInt = std::round(val * 100.);
							app.fastSlowModeSpeedOption() = valAsInt;
							fastSlowModeSpeed.setSelected((MenuItem::Id)valAsInt, *this);
							dismissPrevious();
							return true;
						}
						else
						{
							app.postErrorMessage("Value not in range");
							return false;
						}
					});
				return false;
			}, MenuItem::DEFAULT_ID
		},
	},
	fastSlowModeSpeed
	{
		"Fast/Slow Mode Speed", &defaultFace(),
		[this](size_t idx, Gfx::Text &t)
		{
			t.resetString(fmt::format("{:.2f}x", app().fastSlowModeSpeedAsDouble()));
			return true;
		},
		(MenuItem::Id)app().fastSlowModeSpeedOption().val,
		fastSlowModeSpeedItem
	},
	performanceMode
	{
		"Performance Mode", &defaultFace(),
		(bool)app().sustainedPerformanceModeOption(),
		"Normal", "Sustained",
		[this](BoolMenuItem &item)
		{
			app().sustainedPerformanceModeOption() = item.flipBoolValue(*this);
		}
	}
{
	if(!customMenu)
	{
		loadStockItems();
	}
}

void SystemOptionView::loadStockItems()
{
	item.emplace_back(&autosaveTimer);
	item.emplace_back(&autosaveInitialSlot);
	item.emplace_back(&confirmOverwriteState);
	item.emplace_back(&fastSlowModeSpeed);
	if(used(performanceMode))
		item.emplace_back(&performanceMode);
}

}
