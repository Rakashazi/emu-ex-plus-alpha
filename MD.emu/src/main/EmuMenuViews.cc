#include <emuframework/OptionView.hh>
#include <emuframework/MenuView.hh>
#include "EmuCheatViews.hh"
#include "internal.hh"
#include "input.h"
#include "io_ctrl.h"

class EmuVideoOptionView : public VideoOptionView
{
	TextMenuItem videoSystemItem[3]
	{
		{"Auto", [](){ optionVideoSystem = 0; }},
		{"NTSC", [](){ optionVideoSystem = 1; }},
		{"PAL", [](){ optionVideoSystem = 2; }},
	};

	MultiChoiceMenuItem videoSystem
	{
		"Video System",
		std::min((uint)optionVideoSystem, 3u),
		videoSystemItem
	};

public:
	EmuVideoOptionView(ViewAttachParams attach): VideoOptionView{attach, true}
	{
		loadStockItems();
		item.emplace_back(&videoSystem);
	}
};

class EmuAudioOptionView : public AudioOptionView
{
	BoolMenuItem smsFM
	{
		"MarkIII FM Sound Unit",
		(bool)optionSmsFM,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			optionSmsFM = item.flipBoolValue(*this);
			config_ym2413_enabled = optionSmsFM;
		}
	};

public:
	EmuAudioOptionView(ViewAttachParams attach): AudioOptionView{attach, true}
	{
		loadStockItems();
		item.emplace_back(&smsFM);
	}
};

class EmuInputOptionView : public TableView
{
	BoolMenuItem sixButtonPad
	{
		"6-button Gamepad",
		(bool)option6BtnPad,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			option6BtnPad = item.flipBoolValue(*this);
			setupMDInput();
		}
	};

	BoolMenuItem multitap
	{
		"4-Player Adapter",
		(bool)usingMultiTap,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			usingMultiTap = item.flipBoolValue(*this);
			setupMDInput();
		}
	};

	TextMenuItem inputPortsItem[4]
	{
		{
			"Auto",
			[]()
			{
				mdInputPortDev[0] = mdInputPortDev[1] = -1;
				setupMDInput();
			}
		},
		{
			"Gamepads",
			[]()
			{
				mdInputPortDev[0] = mdInputPortDev[1] = SYSTEM_MD_GAMEPAD;
				setupMDInput();
			}
		},
		{
			"Menacer",
			[]()
			{
				mdInputPortDev[0] = SYSTEM_MD_GAMEPAD; mdInputPortDev[1] = SYSTEM_MENACER;
				setupMDInput();
			}
		},
		{
			"Justifier",
			[]()
			{
				mdInputPortDev[0] = SYSTEM_MD_GAMEPAD; mdInputPortDev[1] = SYSTEM_JUSTIFIER;
				setupMDInput();
			}
		},
	};

	MultiChoiceMenuItem inputPorts
	{
		"Input Ports",
		[]() -> uint
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

public:
	EmuInputOptionView(ViewAttachParams attach):
		TableView
		{
			"Input Options",
			attach,
			[this](const TableView &)
			{
				return 3;
			},
			[this](const TableView &, uint idx) -> MenuItem&
			{
				switch(idx)
				{
					case 0: return inputPorts;
					case 1: return sixButtonPad;
					default: return multitap;
				}
			}
		}
	{}
};

class EmuSystemOptionView : public SystemOptionView
{
	BoolMenuItem bigEndianSram
	{
		"Use Big-Endian SRAM",
		(bool)optionBigEndianSram,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			auto &ynAlertView = *new YesNoAlertView{attachParams(),
				"Warning, this changes the format of SRAM saves files. "
				"Turn on to make them compatible with other emulators like Gens. "
				"Any SRAM loaded with the incorrect setting will be corrupted."};
			ynAlertView.setOnYes(
				[this, &item](TextMenuItem &, View &view, Input::Event e)
				{
					view.dismiss();
					optionBigEndianSram = item.flipBoolValue(*this);
				});
			EmuApp::pushAndShowModalView(ynAlertView, e);
		}
	};

	TextMenuItem videoSystemItem[4]
	{
		{"Auto", [](){ optionRegion = 0; config.region_detect = 0; }},
		{"USA", [](){ optionRegion = 1; config.region_detect = 1; }},
		{"Europe", [](){ optionRegion = 2; config.region_detect = 2; }},
		{"Japan", [](){ optionRegion = 3; config.region_detect = 3; }},
	};

	MultiChoiceMenuItem region
	{
		"Game Region",
		std::min((uint)config.region_detect, 4u),
		videoSystemItem
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

	char cdBiosPathStr[3][256]{};
	TextMenuItem cdBiosPath[3]
	{
		{cdBiosPathStr[0], [this](TextMenuItem &, View &, Input::Event e){ cdBiosPathHandler(e, REGION_USA); }},
		{cdBiosPathStr[1], [this](TextMenuItem &, View &, Input::Event e){ cdBiosPathHandler(e, REGION_JAPAN_NTSC); }},
		{cdBiosPathStr[2], [this](TextMenuItem &, View &, Input::Event e){ cdBiosPathHandler(e, REGION_EUROPE); }}
	};

	template <size_t S>
	static void printBiosMenuEntryStr(char (&str)[S], int region)
	{
		const char *path = "";
		switch(region)
		{
			bdefault: path = cdBiosUSAPath.data();
			bcase REGION_JAPAN_NTSC: path = cdBiosJpnPath.data();
			bcase REGION_EUROPE: path = cdBiosEurPath.data();
		}
		const char *regionStr = biosHeadingStr[regionCodeToIdx(region)];
		string_printf(str, "%s: %s", regionStr, strlen(path) ? FS::basename(path).data() : "None set");
	}

	void cdBiosPathHandler(Input::Event e, int region)
	{
		auto &biosSelectMenu = *new BiosSelectMenu{biosHeadingStr[regionCodeToIdx(region)], &regionCodeToStrBuffer(region),
			[this, region]()
			{
				auto idx = regionCodeToIdx(region);
				logMsg("set bios at idx %d to %s", idx, regionCodeToStrBuffer(region).data());
				printBiosMenuEntryStr(cdBiosPathStr[idx], region);
				cdBiosPath[idx].compile(renderer(), projP);
			},
			hasMDExtension, attachParams()};
		pushAndShow(biosSelectMenu, e);
	}

	void cdBiosPathInit()
	{
		const int region[3]{REGION_USA, REGION_JAPAN_NTSC, REGION_EUROPE};
		iterateTimes(3, i)
		{
			printBiosMenuEntryStr(cdBiosPathStr[i], region[i]);
			item.emplace_back(&cdBiosPath[i]);
		}
	}
	#endif

public:
	EmuSystemOptionView(ViewAttachParams attach): SystemOptionView{attach, true}
	{
		loadStockItems();
		item.emplace_back(&bigEndianSram);
		item.emplace_back(&region);
		#ifndef NO_SCD
		cdBiosPathInit();
		#endif
	}
};

#ifndef NO_SCD
constexpr const char *EmuSystemOptionView::biosHeadingStr[3];
#endif

View *EmuSystem::makeView(ViewAttachParams attach, ViewID id)
{
	switch(id)
	{
		case ViewID::MAIN_MENU: return new MenuView(attach);
		case ViewID::VIDEO_OPTIONS: return new EmuVideoOptionView(attach);
		case ViewID::AUDIO_OPTIONS: return new EmuAudioOptionView(attach);
		case ViewID::INPUT_OPTIONS: return new EmuInputOptionView(attach);
		case ViewID::SYSTEM_OPTIONS: return new EmuSystemOptionView(attach);
		case ViewID::GUI_OPTIONS: return new GUIOptionView(attach);
		case ViewID::EDIT_CHEATS: return new EmuEditCheatListView(attach);
		case ViewID::LIST_CHEATS: return new EmuCheatsView(attach);
		default: return nullptr;
	}
}
