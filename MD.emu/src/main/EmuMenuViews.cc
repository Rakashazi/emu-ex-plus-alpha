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
#include "EmuCheatViews.hh"
#include "internal.hh"
#include <imagine/gui/AlertView.hh>
#include <imagine/fs/FS.hh>
#include <imagine/util/format.hh>
#include "input.h"
#include "io_ctrl.h"
#include "vdp_ctrl.h"

namespace EmuEx
{

class ConsoleOptionView : public TableView, public EmuAppHelper<ConsoleOptionView>
{
	BoolMenuItem sixButtonPad
	{
		"6-button Gamepad", &defaultFace(),
		(bool)option6BtnPad,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			system().sessionOptionSet();
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
		{"Auto",      &defaultFace(), setInputPortsDel(-1, -1), -1},
		{"Gamepads",  &defaultFace(), setInputPortsDel(SYSTEM_MD_GAMEPAD, SYSTEM_MD_GAMEPAD), SYSTEM_MD_GAMEPAD},
		{"Menacer",   &defaultFace(), setInputPortsDel(SYSTEM_MD_GAMEPAD, SYSTEM_MENACER),    SYSTEM_MENACER},
		{"Justifier", &defaultFace(), setInputPortsDel(SYSTEM_MD_GAMEPAD, SYSTEM_JUSTIFIER),  SYSTEM_JUSTIFIER},
	};

	MultiChoiceMenuItem inputPorts
	{
		"Input Ports", &defaultFace(),
		(MenuItem::Id)mdInputPortDev[1],
		inputPortsItem
	};

	TextMenuItem::SelectDelegate setInputPortsDel(int8_t port1, int8_t port2)
	{
		return [this, port1, port2]()
		{
			system().sessionOptionSet();
			optionInputPort1 = mdInputPortDev[0] = port1;
			optionInputPort2 = mdInputPortDev[1] = port2;
			setupMDInput(app());
		};
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
		optionVideoSystem.val,
		videoSystemItem
	};

	void setVideoSystem(int val, Input::Event e)
	{
		system().sessionOptionSet();
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
		system().sessionOptionSet();
		optionRegion = val;
		app().promptSystemReloadDueToSetOption(attachParams(), e);
	}

	StaticArrayList<MenuItem*, 5> item{};

public:
	ConsoleOptionView(ViewAttachParams attach):
		TableView
		{
			"Console Options",
			attach,
			item
		}
	{
		if(system_hw != SYSTEM_PBC)
		{
			item.emplace_back(&inputPorts);
			item.emplace_back(&sixButtonPad);
			item.emplace_back(&multitap);
		}
		item.emplace_back(&videoSystem);
		item.emplace_back(&region);
	}
};

class CustomSystemActionsView : public EmuSystemActionsView
{
private:
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

public:
	CustomSystemOptionView(ViewAttachParams attach): SystemOptionView{attach, true}
	{
		loadStockItems();
		item.emplace_back(&bigEndianSram);
	}
};

class CustomFilePathOptionView : public FilePathOptionView
{
	#ifndef NO_SCD
	static constexpr std::string_view biosHeadingStr[3]
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

	auto biosMenuEntryStr(int region, std::string_view displayName) const
	{
		auto regionStr = biosHeadingStr[regionCodeToIdx(region)];
		return fmt::format("{}: {}", regionStr, displayName);
	}

	void cdBiosPathHandler(Input::Event e, int region)
	{
		auto biosSelectMenu = makeViewWithName<BiosSelectMenu>(biosHeadingStr[regionCodeToIdx(region)], &regionCodeToStrBuffer(region),
			[this, region](std::string_view displayName)
			{
				auto idx = regionCodeToIdx(region);
				logMsg("set bios at idx %d to %s", idx, regionCodeToStrBuffer(region).data());
				cdBiosPath[idx].compile(biosMenuEntryStr(region, displayName), renderer(), projP);
			},
			hasMDExtension);
		pushAndShow(std::move(biosSelectMenu), e);
	}

	void cdBiosPathInit()
	{
		static constexpr int regions[3]{REGION_USA, REGION_JAPAN_NTSC, REGION_EUROPE};
		for(int i = 0; auto r : regions)
		{
			cdBiosPath[i].setName(biosMenuEntryStr(r, appContext().fileUriDisplayName(regionCodeToStrBuffer(r))));
			item.emplace_back(&cdBiosPath[i]);
			i++;
		}
	}
	#endif

	public:
	CustomFilePathOptionView(ViewAttachParams attach): FilePathOptionView{attach, true}
	{
		loadStockItems();
		#ifndef NO_SCD
		cdBiosPathInit();
		#endif
	}
};

std::unique_ptr<View> EmuApp::makeCustomView(ViewAttachParams attach, ViewID id)
{
	switch(id)
	{
		case ViewID::AUDIO_OPTIONS: return std::make_unique<CustomAudioOptionView>(attach);
		case ViewID::SYSTEM_ACTIONS: return std::make_unique<CustomSystemActionsView>(attach);
		case ViewID::SYSTEM_OPTIONS: return std::make_unique<CustomSystemOptionView>(attach);
		case ViewID::FILE_PATH_OPTIONS: return std::make_unique<CustomFilePathOptionView>(attach);
		case ViewID::EDIT_CHEATS: return std::make_unique<EmuEditCheatListView>(attach);
		case ViewID::LIST_CHEATS: return std::make_unique<EmuCheatsView>(attach);
		default: return nullptr;
	}
}

}
