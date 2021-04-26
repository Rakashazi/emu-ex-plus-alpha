/*  This file is part of MD.emu.

	MD.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MD.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with MD.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuApp.hh>
#include <emuframework/EmuAppHelper.hh>
#include <emuframework/OptionView.hh>
#include <emuframework/EmuSystemActionsView.hh>
#include <imagine/gui/AlertView.hh>
#include "EmuCheatViews.hh"
#include "internal.hh"
#include "input.h"
#include "io_ctrl.h"
#include "vdp_ctrl.h"

class ConsoleOptionView : public TableView, public EmuAppHelper<ConsoleOptionView>
{
	BoolMenuItem sixButtonPad
	{
		"6-button Gamepad", &defaultFace(),
		(bool)option6BtnPad,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			EmuSystem::sessionOptionSet();
			option6BtnPad = item.flipBoolValue(*this);
			setupMDInput(app());
		}
	};

	BoolMenuItem multitap
	{
		"4-Player Adapter", &defaultFace(),
		(bool)optionMultiTap,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			optionMultiTap = item.flipBoolValue(*this);
			setupMDInput(app());
		}
	};

	TextMenuItem inputPortsItem[4]
	{
		{"Auto", &defaultFace(), [this](){ setInputPorts(-1, -1); }},
		{"Gamepads", &defaultFace(), [this]() { setInputPorts(SYSTEM_MD_GAMEPAD, SYSTEM_MD_GAMEPAD); }},
		{"Menacer", &defaultFace(), [this]() { setInputPorts(SYSTEM_MD_GAMEPAD, SYSTEM_MENACER); }},
		{"Justifier", &defaultFace(), [this]() { setInputPorts(SYSTEM_MD_GAMEPAD, SYSTEM_JUSTIFIER); }},
	};

	MultiChoiceMenuItem inputPorts
	{
		"Input Ports", &defaultFace(),
		[]()
		{
			if(mdInputPortDev[0] == SYSTEM_MD_GAMEPAD && mdInputPortDev[1] == SYSTEM_MD_GAMEPAD)
				return 1;
			else if(mdInputPortDev[0] == SYSTEM_MD_GAMEPAD && mdInputPortDev[1] == SYSTEM_MENACER)
				return 2;
			else if(mdInputPortDev[0] == SYSTEM_MD_GAMEPAD && mdInputPortDev[1] == SYSTEM_JUSTIFIER)
				return 3;
			else
				return 0;
		}(),
		inputPortsItem
	};

	void setInputPorts(int port1, int port2)
	{
		EmuSystem::sessionOptionSet();
		optionInputPort1 = mdInputPortDev[0] = port1;
		optionInputPort2 = mdInputPortDev[1] = port2;
		setupMDInput(app());
	}

	TextMenuItem videoSystemItem[3]
	{
		{"Auto", &defaultFace(), [this](TextMenuItem &, View &, Input::Event e){ setVideoSystem(0, e); }},
		{"NTSC", &defaultFace(), [this](TextMenuItem &, View &, Input::Event e){ setVideoSystem(1, e); }},
		{"PAL", &defaultFace(), [this](TextMenuItem &, View &, Input::Event e){ setVideoSystem(2, e); }},
	};

	MultiChoiceMenuItem videoSystem
	{
		"Video System", &defaultFace(),
		[this](int idx, Gfx::Text &t)
		{
			if(idx == 0)
			{
				t.setString(vdp_pal ? "PAL" : "NTSC");
				return true;
			}
			return false;
		},
		optionVideoSystem,
		videoSystemItem
	};

	void setVideoSystem(int val, Input::Event e)
	{
		EmuSystem::sessionOptionSet();
		optionVideoSystem = val;
		app().promptSystemReloadDueToSetOption(attachParams(), e);
	}

	TextMenuItem regionItem[4]
	{
		{"Auto", &defaultFace(), [this](TextMenuItem &, View &, Input::Event e){ setRegion(0, e); }},
		{"USA", &defaultFace(), [this](TextMenuItem &, View &, Input::Event e){ setRegion(1, e); }},
		{"Europe", &defaultFace(), [this](TextMenuItem &, View &, Input::Event e){ setRegion(2, e); }},
		{"Japan", &defaultFace(), [this](TextMenuItem &, View &, Input::Event e){ setRegion(3, e); }},
	};

	MultiChoiceMenuItem region
	{
		"Game Region", &defaultFace(),
		[this](int idx, Gfx::Text &t)
		{
			if(idx == 0)
			{
				auto regionStr = [](unsigned region)
				{
					switch(region)
					{
						case REGION_USA: return "USA";
						case REGION_EUROPE: return "Europe";
						default: return "Japan";
					}
				};
				t.setString(regionStr(region_code));
				return true;
			}
			return false;
		},
		std::min((int)config.region_detect, 4),
		regionItem
	};

	void setRegion(int val, Input::Event e)
	{
		EmuSystem::sessionOptionSet();
		optionRegion = val;
		app().promptSystemReloadDueToSetOption(attachParams(), e);
	}

	std::array<MenuItem*, 5> menuItem
	{
		&inputPorts,
		&sixButtonPad,
		&multitap,
		&videoSystem,
		&region
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
private:
	TextMenuItem options
	{
		"Console Options", &defaultFace(),
		[this](TextMenuItem &, View &, Input::Event e)
		{
			if(EmuSystem::gameIsRunning())
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

class CustomAudioOptionView : public AudioOptionView
{
	BoolMenuItem smsFM
	{
		"MarkIII FM Sound Unit", &defaultFace(),
		(bool)optionSmsFM,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			optionSmsFM = item.flipBoolValue(*this);
			config_ym2413_enabled = optionSmsFM;
		}
	};

public:
	CustomAudioOptionView(ViewAttachParams attach): AudioOptionView{attach, true}
	{
		loadStockItems();
		item.emplace_back(&smsFM);
	}
};

class CustomSystemOptionView : public SystemOptionView
{
	BoolMenuItem bigEndianSram
	{
		"Use Big-Endian SRAM", &defaultFace(),
		(bool)optionBigEndianSram,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			auto ynAlertView = makeView<YesNoAlertView>(
				"Warning, this changes the format of SRAM saves files. "
				"Turn on to make them compatible with other emulators like Gens. "
				"Any SRAM loaded with the incorrect setting will be corrupted.");
			ynAlertView->setOnYes(
				[this, &item]()
				{
					optionBigEndianSram = item.flipBoolValue(*this);
				});
			app().pushAndShowModalView(std::move(ynAlertView), e);
		}
	};

	#ifndef NO_SCD
	static constexpr const char *biosHeadingStr[3]
	{
		"USA CD BIOS",
		"Japan CD BIOS",
		"Europe CD BIOS"
	};

	static int regionCodeToIdx(int region)
	{
		switch(region)
		{
			default: return 0;
			case REGION_JAPAN_NTSC: return 1;
			case REGION_EUROPE: return 2;
		}
	}

	static FS::PathString &regionCodeToStrBuffer(int region)
	{
		switch(region)
		{
			default: return cdBiosUSAPath;
			case REGION_JAPAN_NTSC: return cdBiosJpnPath;
			case REGION_EUROPE: return cdBiosEurPath;
		}
	}

	TextMenuItem cdBiosPath[3]
	{
		{{}, &defaultFace(), [this](Input::Event e){ cdBiosPathHandler(e, REGION_USA); }},
		{{}, &defaultFace(), [this](Input::Event e){ cdBiosPathHandler(e, REGION_JAPAN_NTSC); }},
		{{}, &defaultFace(), [this](Input::Event e){ cdBiosPathHandler(e, REGION_EUROPE); }}
	};

	static std::array<char, 256> makeBiosMenuEntryStr(int region)
	{
		const char *path = "";
		switch(region)
		{
			bdefault: path = cdBiosUSAPath.data();
			bcase REGION_JAPAN_NTSC: path = cdBiosJpnPath.data();
			bcase REGION_EUROPE: path = cdBiosEurPath.data();
		}
		const char *regionStr = biosHeadingStr[regionCodeToIdx(region)];
		return string_makePrintf<256>("%s: %s", regionStr, strlen(path) ? FS::basename(path).data() : "None set");
	}

	void cdBiosPathHandler(Input::Event e, int region)
	{
		auto biosSelectMenu = makeViewWithName<BiosSelectMenu>(biosHeadingStr[regionCodeToIdx(region)], &regionCodeToStrBuffer(region),
			[this, region]()
			{
				auto idx = regionCodeToIdx(region);
				logMsg("set bios at idx %d to %s", idx, regionCodeToStrBuffer(region).data());
				cdBiosPath[idx].setName(makeBiosMenuEntryStr(region).data());
				cdBiosPath[idx].compile(renderer(), projP);
			},
			hasMDExtension);
		pushAndShow(std::move(biosSelectMenu), e);
	}

	void cdBiosPathInit()
	{
		const int region[3]{REGION_USA, REGION_JAPAN_NTSC, REGION_EUROPE};
		iterateTimes(3, i)
		{
			cdBiosPath[i].setName(makeBiosMenuEntryStr(region[i]).data());
			item.emplace_back(&cdBiosPath[i]);
		}
	}
	#endif

public:
	CustomSystemOptionView(ViewAttachParams attach): SystemOptionView{attach, true}
	{
		loadStockItems();
		item.emplace_back(&bigEndianSram);
		#ifndef NO_SCD
		cdBiosPathInit();
		#endif
	}
};

#ifndef NO_SCD
constexpr const char *CustomSystemOptionView::biosHeadingStr[3];
#endif

std::unique_ptr<View> EmuApp::makeCustomView(ViewAttachParams attach, ViewID id)
{
	switch(id)
	{
		case ViewID::AUDIO_OPTIONS: return std::make_unique<CustomAudioOptionView>(attach);
		case ViewID::SYSTEM_ACTIONS: return std::make_unique<CustomSystemActionsView>(attach);
		case ViewID::SYSTEM_OPTIONS: return std::make_unique<CustomSystemOptionView>(attach);
		case ViewID::EDIT_CHEATS: return std::make_unique<EmuEditCheatListView>(attach);
		case ViewID::LIST_CHEATS: return std::make_unique<EmuCheatsView>(attach);
		default: return nullptr;
	}
}
