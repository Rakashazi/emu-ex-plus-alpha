#pragma once
#include <emuframework/OptionView.hh>
#include <emuframework/MenuView.hh>
#include "EmuCheatViews.hh"

static void setupMDInput();

class SystemOptionView : public OptionView
{
public:

	BoolMenuItem sixButtonPad
	{
		[this](BoolMenuItem &item, View &, const Input::Event &e)
		{
			item.toggle(*this);
			option6BtnPad = item.on;
			setupMDInput();
			#ifdef CONFIG_VCONTROLS_GAMEPAD
			EmuControls::setupVControllerVars();
			vController.place();
			#endif
		}
	},
	multitap
	{
		[this](BoolMenuItem &item, View &, const Input::Event &e)
		{
			item.toggle(*this);
			usingMultiTap = item.on;
			setupMDInput();
		}
	},
	smsFM
	{
		[this](BoolMenuItem &item, View &, const Input::Event &e)
		{
			item.toggle(*this);
			optionSmsFM = item.on;
			config_ym2413_enabled = optionSmsFM;
		}
	},
	bigEndianSram
	{
		[this](BoolMenuItem &item, View &, const Input::Event &e)
		{
			auto &ynAlertView = *new YesNoAlertView{window()};
			ynAlertView.init("Warning, this changes the format of SRAM saves files. "
					"Turn on to make them compatible with other emulators like Gens. "
					"Any SRAM loaded with the incorrect setting will be corrupted.", !e.isPointer());
			ynAlertView.onYes() =
				[this, &item](const Input::Event &e)
				{
					item.toggle(*this);
					optionBigEndianSram = item.on;
				};
			modalViewController.pushAndShow(ynAlertView);
		}
	};

	MultiChoiceSelectMenuItem region
	{
		"Game Region",
		[](MultiChoiceMenuItem &, View &, int val)
		{
			optionRegion = val;
			config.region_detect = val;
		}
	};

	void regionInit()
	{
		static const char *str[] =
		{
			"Auto", "USA", "Europe", "Japan"
		};
		int setting = 0;
		if(config.region_detect < 4)
		{
			setting = config.region_detect;
		}

		region.init(str, setting, sizeofArray(str));
	}

	#ifndef NO_SCD
	static constexpr const char *biosHeadingStr[3] = { "USA CD BIOS", "Japan CD BIOS", "Europe CD BIOS" };

	static int regionCodeToIdx(int region)
	{
		switch(region)
		{
			default: return 0;
			case REGION_JAPAN_NTSC: return 1;
			case REGION_EUROPE: return 2;
		}
	}

	static FsSys::PathString &regionCodeToStrBuffer(int region)
	{
		switch(region)
		{
			default: return cdBiosUSAPath;
			case REGION_JAPAN_NTSC: return cdBiosJpnPath;
			case REGION_EUROPE: return cdBiosEurPath;
		}
	}

	char cdBiosPathStr[3][256] { {0} };
	TextMenuItem cdBiosPath[3]
	{
		{ [this](TextMenuItem &item, View &, const Input::Event &e){ cdBiosPathHandler(e, REGION_USA); } },
		{ [this](TextMenuItem &item, View &, const Input::Event &e){ cdBiosPathHandler(e, REGION_JAPAN_NTSC); } },
		{ [this](TextMenuItem &item, View &, const Input::Event &e){ cdBiosPathHandler(e, REGION_EUROPE); } }
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
		FsSys::PathString basenameTemp;
		string_printf(str, "%s: %s", regionStr, strlen(path) ? string_basename(path, basenameTemp) : "None set");
	}

	void cdBiosPathHandler(const Input::Event &e, int region)
	{
		auto &biosSelectMenu = *new BiosSelectMenu{biosHeadingStr[regionCodeToIdx(region)], &regionCodeToStrBuffer(region), mdROMFsFilter, window()};
		biosSelectMenu.init(!e.isPointer());
		biosSelectMenu.onBiosChange() =
			[this, region]()
			{
				auto idx = regionCodeToIdx(region);
				logMsg("set bios at idx %d to %s", idx, regionCodeToStrBuffer(region).data());
				printBiosMenuEntryStr(cdBiosPathStr[idx], region);
				cdBiosPath[idx].compile(projP);
			};
		viewStack.pushAndShow(biosSelectMenu);
	}

	void cdBiosPathInit(MenuItem *item[], uint &items)
	{
		const int region[3] = { REGION_USA, REGION_JAPAN_NTSC, REGION_EUROPE };
		iterateTimes(3, i)
		{
			printBiosMenuEntryStr(cdBiosPathStr[i], region[i]);
			cdBiosPath[i].init(cdBiosPathStr[i]); item[items++] = &cdBiosPath[i];
		}
	}
	#endif

	MultiChoiceSelectMenuItem inputPorts
	{
		"Input Ports",
		[](MultiChoiceMenuItem &, View &, int val)
		{
			if(val == 0)
			{
				mdInputPortDev[0] = mdInputPortDev[1] = -1;
			}
			else if(val == 1)
			{
				mdInputPortDev[0] = mdInputPortDev[1] = SYSTEM_MD_GAMEPAD;
			}
			else if(val == 2)
			{
				mdInputPortDev[0] = SYSTEM_MD_GAMEPAD; mdInputPortDev[1] = SYSTEM_MENACER;
			}
			else if(val == 3)
			{
				mdInputPortDev[0] = SYSTEM_MD_GAMEPAD; mdInputPortDev[1] = SYSTEM_JUSTIFIER;
			}

			setupMDInput();
		}
	};

	void inputPortsInit()
	{
		static const char *str[] =
		{
			"Auto", "Gamepads", "Menacer", "Justifier"
		};
		int setting = 0;
		if(mdInputPortDev[0] == SYSTEM_MD_GAMEPAD && mdInputPortDev[1] == SYSTEM_MD_GAMEPAD)
			setting = 1;
		else if(mdInputPortDev[0] == SYSTEM_MD_GAMEPAD && mdInputPortDev[1] == SYSTEM_MENACER)
			setting = 2;
		else if(mdInputPortDev[0] == SYSTEM_MD_GAMEPAD && mdInputPortDev[1] == SYSTEM_JUSTIFIER)
			setting = 3;

		inputPorts.init(str, setting, sizeofArray(str));
	}

	MultiChoiceSelectMenuItem videoSystem
	{
		"Video System",
		[](MultiChoiceMenuItem &, View &, int val)
		{
			optionVideoSystem = val;
		}
	};

	void videoSystemInit()
	{
		static const char *str[] =
		{
			"Auto", "NTSC", "PAL"
		};
		videoSystem.init(str, std::min((int)optionVideoSystem, (int)sizeofArray(str)-1), sizeofArray(str));
	}

public:
	SystemOptionView(Base::Window &win):
		OptionView(win)
	{}

	void loadVideoItems(MenuItem *item[], uint &items)
	{
		OptionView::loadVideoItems(item, items);
		videoSystemInit(); item[items++] = &videoSystem;
	}

	void loadAudioItems(MenuItem *item[], uint &items)
	{
		OptionView::loadAudioItems(item, items);
		smsFM.init("MarkIII FM Sound Unit", optionSmsFM); item[items++] = &smsFM;
	}

	void loadInputItems(MenuItem *item[], uint &items)
	{
		OptionView::loadInputItems(item, items);
		inputPortsInit(); item[items++] = &inputPorts;
		sixButtonPad.init("6-button Gamepad", option6BtnPad); item[items++] = &sixButtonPad;
		multitap.init("4-Player Adapter", usingMultiTap); item[items++] = &multitap;
	}

	void loadSystemItems(MenuItem *item[], uint &items)
	{
		OptionView::loadSystemItems(item, items);
		bigEndianSram.init("Use Big-Endian SRAM", optionBigEndianSram); item[items++] = &bigEndianSram;
		regionInit(); item[items++] = &region;
		#ifndef NO_SCD
		cdBiosPathInit(item, items);
		#endif
	}

	void init(uint idx, bool highlightFirst)
	{
		uint i = 0;
		switch(idx)
		{
			bcase 0: loadVideoItems(item, i);
			bcase 1: loadAudioItems(item, i);
			bcase 2: loadInputItems(item, i);
			bcase 3: loadSystemItems(item, i);
			bcase 4: loadGUIItems(item, i);
		}
		assert(i <= sizeofArray(item));
		TableView::init(item, i, highlightFirst);
	}
};

#ifndef NO_SCD
constexpr const char *SystemOptionView::biosHeadingStr[3];
#endif

class SystemMenuView : public MenuView
{
	TextMenuItem cheats
	{
		"Cheats",
		[this](TextMenuItem &item, View &, const Input::Event &e)
		{
			if(EmuSystem::gameIsRunning())
			{
				auto &cheatsMenu = *new CheatsView{window()};
				cheatsMenu.init(!e.isPointer());
				viewStack.pushAndShow(cheatsMenu);
			}
		}
	};

public:
	SystemMenuView(Base::Window &win): MenuView(win) {}

	void onShow()
	{
		MenuView::onShow();
		cheats.active = EmuSystem::gameIsRunning();
	}

	void init(bool highlightFirst)
	{
		name_ = appViewTitle();
		uint items = 0;
		loadFileBrowserItems(item, items);
		cheats.init(); item[items++] = &cheats;
		loadStandardItems(item, items);
		assert(items <= sizeofArray(item));
		TableView::init(item, items, highlightFirst);
	}
};
