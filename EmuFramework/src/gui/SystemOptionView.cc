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
#include "../EmuOptions.hh"
#include "CPUAffinityView.hh"
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
		{"Off",    &defaultFace(), 0},
		{"5mins",  &defaultFace(), 5},
		{"10mins", &defaultFace(), 10},
		{"15mins", &defaultFace(), 15},
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
	autosaveContent
	{
		"Autosave Content", &defaultFace(),
		app().autosaveManager().saveOnlyBackupMemory,
		"State & Backup RAM", "Only Backup RAM",
		[this](BoolMenuItem &item)
		{
			app().autosaveManager().saveOnlyBackupMemory = item.flipBoolValue(*this);
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
	fastModeSpeedItem
	{
		{"1.5x",  &defaultFace(), 150},
		{"2x",    &defaultFace(), 200},
		{"4x",    &defaultFace(), 400},
		{"8x",    &defaultFace(), 800},
		{"16x",   &defaultFace(), 1600},
		{"Custom Value", &defaultFace(),
			[this](const Input::Event &e)
			{
				app().pushAndShowNewCollectValueInputView<float>(attachParams(), e, "Input above 1.0 to 20.0", "",
					[this](EmuApp &app, auto val)
					{
						auto valAsInt = std::round(val * 100.f);
						if(app.setAltSpeed(AltSpeedMode::fast, valAsInt))
						{
							fastModeSpeed.setSelected((MenuItem::Id)valAsInt, *this);
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
	fastModeSpeed
	{
		"Fast-forward Speed", &defaultFace(),
		{
			.onSetDisplayString = [this](auto idx, Gfx::Text &t)
			{
				t.resetString(fmt::format("{:g}x", app().altSpeedAsDouble(AltSpeedMode::fast)));
				return true;
			},
			.defaultItemOnSelect = [this](TextMenuItem &item) { app().setAltSpeed(AltSpeedMode::fast, item.id()); }
		},
		(MenuItem::Id)app().altSpeed(AltSpeedMode::fast),
		fastModeSpeedItem
	},
	slowModeSpeedItem
	{
		{"0.25x", &defaultFace(), 25},
		{"0.50x", &defaultFace(), 50},
		{"Custom Value", &defaultFace(),
			[this](const Input::Event &e)
			{
				app().pushAndShowNewCollectValueInputView<float>(attachParams(), e, "Input 0.05 up to 1.0", "",
					[this](EmuApp &app, auto val)
					{
						auto valAsInt = std::round(val * 100.f);
						if(app.setAltSpeed(AltSpeedMode::slow, valAsInt))
						{
							slowModeSpeed.setSelected((MenuItem::Id)valAsInt, *this);
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
	slowModeSpeed
	{
		"Slow-motion Speed", &defaultFace(),
		{
			.onSetDisplayString = [this](auto idx, Gfx::Text &t)
			{
				t.resetString(fmt::format("{:g}x", app().altSpeedAsDouble(AltSpeedMode::slow)));
				return true;
			},
			.defaultItemOnSelect = [this](TextMenuItem &item) { app().setAltSpeed(AltSpeedMode::slow, item.id()); }
		},
		(MenuItem::Id)app().altSpeed(AltSpeedMode::slow),
		slowModeSpeedItem
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
	},
	cpuAffinity
	{
		"Override CPU Affinity", &defaultFace(),
		[this](const Input::Event &e)
		{
			pushAndShow(makeView<CPUAffinityView>(appContext().cpuCount()), e);
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
	item.emplace_back(&autosaveContent);
	item.emplace_back(&confirmOverwriteState);
	item.emplace_back(&fastModeSpeed);
	item.emplace_back(&slowModeSpeed);
	if(used(performanceMode))
		item.emplace_back(&performanceMode);
	if(used(cpuAffinity) && appContext().cpuCount() > 1)
		item.emplace_back(&cpuAffinity);
}

}
