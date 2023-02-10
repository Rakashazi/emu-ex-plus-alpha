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

SystemOptionView::SystemOptionView(ViewAttachParams attach, bool customMenu):
	TableView{"System Options", attach, item},
	autosaveTimerItem
	{
		{"Only On Exit",     &defaultFace(), 0},
		{"5mins & On Exit",  &defaultFace(), 5},
		{"10mins & On Exit", &defaultFace(), 10},
		{"15mins & On Exit", &defaultFace(), 15},
	},
	autosaveTimer
	{
		"Autosave Timer", &defaultFace(),
		{
			.defaultItemOnSelect = [this](TextMenuItem &item) { app().autosaveManager().autosaveTimerMins = IG::Minutes{item.id()}; }
		},
		(MenuItem::Id)app().autosaveManager().autosaveTimerMins.count(),
		autosaveTimerItem
	},
	autosaveLaunchItem
	{
		{"Main Slot",            &defaultFace(), to_underlying(AutosaveLaunchMode::Load)},
		{"Main Slot (No State)", &defaultFace(), to_underlying(AutosaveLaunchMode::LoadNoState)},
		{"No Save Slot",         &defaultFace(), to_underlying(AutosaveLaunchMode::NoSave)},
		{"Select Slot",          &defaultFace(), to_underlying(AutosaveLaunchMode::Ask)},
	},
	autosaveLaunch
	{
		"Autosave Launch Mode", &defaultFace(),
		{
			.defaultItemOnSelect = [this](TextMenuItem &item) { app().autosaveManager().autosaveLaunchMode = AutosaveLaunchMode(item.id()); }
		},
		(MenuItem::Id)app().autosaveManager().autosaveLaunchMode,
		autosaveLaunchItem
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
		{"0.25x", &defaultFace(), 25},
		{"0.50x", &defaultFace(), 50},
		{"1.5x",  &defaultFace(), 150},
		{"2x",    &defaultFace(), 200},
		{"4x",    &defaultFace(), 400},
		{"8x",    &defaultFace(), 800},
		{"16x",   &defaultFace(), 1600},
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
		{
			.onSetDisplayString = [this](auto idx, Gfx::Text &t)
			{
				t.resetString(fmt::format("{:.2f}x", app().fastSlowModeSpeedAsDouble()));
				return true;
			},
			.defaultItemOnSelect = [this](TextMenuItem &item) { app().fastSlowModeSpeedOption() = item.id(); }
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
	item.emplace_back(&autosaveLaunch);
	item.emplace_back(&autosaveTimer);
	item.emplace_back(&confirmOverwriteState);
	item.emplace_back(&fastSlowModeSpeed);
	if(used(performanceMode))
		item.emplace_back(&performanceMode);
}

}
