/*  This file is part of GBA.emu.

	GBA.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	GBA.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with GBA.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuApp.hh>
#include <emuframework/OptionView.hh>
#include <emuframework/EmuSystemActionsView.hh>
#include "EmuCheatViews.hh"
#include "MainApp.hh"
#include <imagine/gui/AlertView.hh>
#include <vbam/gba/GBA.h>
#include <vbam/gba/RTC.h>

namespace EmuEx
{

template <class T>
using MainAppHelper = EmuAppHelper<T, MainApp>;

class ConsoleOptionView : public TableView, public MainAppHelper<ConsoleOptionView>
{
	TextMenuItem rtcItem[3]
	{
		{"Auto", &defaultFace(), [this](){ setRTCEmulation(RtcMode::AUTO); }},
		{"Off",  &defaultFace(), [this](){ setRTCEmulation(RtcMode::OFF); }},
		{"On",   &defaultFace(), [this](){ setRTCEmulation(RtcMode::ON); }},
	};

	MultiChoiceMenuItem rtc
	{
		"RTC Emulation", &defaultFace(),
		[this](int idx, Gfx::Text &t)
		{
			if(idx == 0)
			{
				t.setString(rtcIsEnabled() ? "On" : "Off");
				return true;
			}
			return false;
		},
		system().optionRtcEmulation.val,
		rtcItem
	};

	void setRTCEmulation(RtcMode val)
	{
		system().sessionOptionSet();
		system().optionRtcEmulation = to_underlying(val);
		system().setRTC(val);
	}

	TextMenuItem saveTypeItem[7]
	{
		{"Auto",            &defaultFace(), setSaveTypeDel(), packSaveTypeOverride(GBA_SAVE_AUTO)},
		{"EEPROM",          &defaultFace(), setSaveTypeDel(), packSaveTypeOverride(GBA_SAVE_EEPROM)},
		{"SRAM",            &defaultFace(), setSaveTypeDel(), packSaveTypeOverride(GBA_SAVE_SRAM)},
		{"Flash (64K)",     &defaultFace(), setSaveTypeDel(), packSaveTypeOverride(GBA_SAVE_FLASH, SIZE_FLASH512)},
		{"Flash (128K)",    &defaultFace(), setSaveTypeDel(), packSaveTypeOverride(GBA_SAVE_FLASH, SIZE_FLASH1M)},
		{"EEPROM + Sensor", &defaultFace(), setSaveTypeDel(), packSaveTypeOverride(GBA_SAVE_EEPROM_SENSOR)},
		{"None",            &defaultFace(), setSaveTypeDel(), packSaveTypeOverride(GBA_SAVE_NONE)},
	};

	MultiChoiceMenuItem saveType
	{
		"Save Type", &defaultFace(),
		[this](int idx, Gfx::Text &t)
		{
			if(idx == 0)
			{
				t.setString(saveTypeStr(system().detectedSaveType, system().detectedSaveSize));
				return true;
			}
			return false;
		},
		(MenuItem::Id)system().optionSaveTypeOverride.val,
		saveTypeItem
	};

	TextMenuItem::SelectDelegate setSaveTypeDel()
	{
		return [this](TextMenuItem &item, const Input::Event &e)
		{
			if(system().optionSaveTypeOverride == (uint32_t)item.id())
				return true;
			static auto setSaveTypeOption = [](GbaApp &app, uint32_t optVal, ViewAttachParams attach, const Input::Event &e)
			{
				app.system().sessionOptionSet();
				app.system().optionSaveTypeOverride = optVal;
				app.promptSystemReloadDueToSetOption(attach, e);
			};
			if(saveMemoryHasContent())
			{
				auto ynAlertView = std::make_unique<YesNoAlertView>(attachParams(),
					"Really change save type? Existing data in .sav file may be lost so please make a backup before proceeding.");
				ynAlertView->setOnYes(
					[this, optVal = item.id()](const Input::Event &e)
					{
						setSaveTypeOption(app(), optVal, attachParams(), e);
					});
				pushAndShowModal(std::move(ynAlertView), e);
				return false;
			}
			else
			{
				setSaveTypeOption(app(), item.id(), attachParams(), e);
				return true;
			}
		};
	}

	std::array<MenuItem*, 2> menuItem
	{
		&rtc,
		&saveType
	};

public:
	ConsoleOptionView(ViewAttachParams attach):
		TableView
		{
			"Console Options",
			attach,
			menuItem
		}
	{}
};

class CustomSystemActionsView : public EmuSystemActionsView
{
	TextMenuItem options
	{
		"Console Options", &defaultFace(),
		[this](TextMenuItem &, View &, Input::Event e)
		{
			if(system().hasContent())
			{
				pushAndShow(makeView<ConsoleOptionView>(), e);
			}
		}
	};

public:
	CustomSystemActionsView(ViewAttachParams attach): EmuSystemActionsView{attach, true}
	{
		item.emplace_back(&options);
		loadStandardItems();
	}
};

std::unique_ptr<View> EmuApp::makeCustomView(ViewAttachParams attach, ViewID id)
{
	switch(id)
	{
		case ViewID::SYSTEM_ACTIONS: return std::make_unique<CustomSystemActionsView>(attach);
		case ViewID::EDIT_CHEATS: return std::make_unique<EmuEditCheatListView>(attach);
		case ViewID::LIST_CHEATS: return std::make_unique<EmuCheatsView>(attach);
		default: return nullptr;
	}
}

}
