#pragma once
#include "OptionView.hh"

static bool isFDSBIOSExtension(const char *name);
static void setupNESInputPorts();
static void setupNESFourScore();

class SystemOptionView : public OptionView
{
public:

	char fdsBiosPathStr[256] {0};
	TextMenuItem fdsBiosPath
	{
		"",
		[this](TextMenuItem &, const Input::Event &e)
		{
			auto &biosSelectMenu = *menuAllocator.allocNew<BiosSelectMenu>("Disk System BIOS", &::fdsBiosPath, biosFsFilter, window());
			biosSelectMenu.init(!e.isPointer());
			biosSelectMenu.onBiosChange() =
				[this]()
				{
					logMsg("set fds bios %s", ::fdsBiosPath);
					printBiosMenuEntryStr(fdsBiosPathStr);
					fdsBiosPath.compile();
				};
			viewStack.pushAndShow(biosSelectMenu, &menuAllocator);
		}
	};

	template <size_t S>
	static void printBiosMenuEntryStr(char (&str)[S])
	{
		FsSys::cPath basenameTemp;
		string_printf(str, "Disk System BIOS: %s", strlen(::fdsBiosPath) ? string_basename(::fdsBiosPath, basenameTemp) : "None set");
	}

	BoolMenuItem fourScore
	{
		"4-Player Adapter",
		[this](BoolMenuItem &item, const Input::Event &e)
		{
			item.toggle(*this);
			optionFourScore = item.on;
			setupNESFourScore();
		}
	};

	MultiChoiceSelectMenuItem inputPorts
	{
		"Input Ports",
		[](MultiChoiceMenuItem &, int val)
		{
			if(val == 0)
			{
				nesInputPortDev[0] = nesInputPortDev[1] = SI_UNSET;
			}
			else if(val == 1)
			{
				nesInputPortDev[0] = nesInputPortDev[1] = SI_GAMEPAD;
			}
			else if(val == 2)
			{
				nesInputPortDev[0] = SI_GAMEPAD; nesInputPortDev[1] = SI_ZAPPER;
			}
			else if(val == 3)
			{
				nesInputPortDev[0] = SI_ZAPPER; nesInputPortDev[1] = SI_GAMEPAD;
			}

			setupNESInputPorts();
		}
	};

	void inputPortsInit()
	{
		static const char *str[] =
		{
			"Auto", "Gamepads", "Gun (2P, NES)", "Gun (1P, VS)"
		};
		int setting = 0;
		if(nesInputPortDev[0] == SI_GAMEPAD && nesInputPortDev[1] == SI_GAMEPAD)
			setting = 1;
		else if(nesInputPortDev[0] == SI_GAMEPAD && nesInputPortDev[1] == SI_ZAPPER)
			setting = 2;
		else if(nesInputPortDev[0] == SI_ZAPPER && nesInputPortDev[1] == SI_GAMEPAD)
			setting = 3;

		inputPorts.init(str, setting, sizeofArray(str));
	}

	MultiChoiceSelectMenuItem videoSystem
	{
		"Video System",
		[](MultiChoiceMenuItem &, int val)
		{
			optionVideoSystem = val;
			switch(val)
			{
				bcase 0:
					logMsg("using %s", autoDetectedVidSysPAL ? "PAL" : "NTSC");
					FCEUI_SetVidSystem(autoDetectedVidSysPAL);
				bcase 1:
					logMsg("forcing NTSC");
					FCEUI_SetVidSystem(0);
				bcase 2:
					logMsg("forcing PAL");
					FCEUI_SetVidSystem(1);
			}
			EmuSystem::configAudioPlayback();
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
	SystemOptionView(Base::Window &win): OptionView(win) {}

	void loadVideoItems(MenuItem *item[], uint &items)
	{
		OptionView::loadVideoItems(item, items);
		videoSystemInit(); item[items++] = &videoSystem;
	}

	void loadInputItems(MenuItem *item[], uint &items)
	{
		OptionView::loadInputItems(item, items);
		inputPortsInit(); item[items++] = &inputPorts;
		fourScore.init(optionFourScore); item[items++] = &fourScore;
	}

	void loadSystemItems(MenuItem *item[], uint &items)
	{
		OptionView::loadSystemItems(item, items);
		printBiosMenuEntryStr(fdsBiosPathStr);
		fdsBiosPath.init(fdsBiosPathStr); item[items++] = &fdsBiosPath;
	}
};

#include "EmuCheatViews.hh"
#include "MenuView.hh"

class FDSControlView : public BaseMenuView
{
private:
	struct SetSideMenuItem : public TextMenuItem
	{
		constexpr SetSideMenuItem() { }
		int side = 0;
		void init(const char *sideStr, int side)
		{
			TextMenuItem::init(sideStr, side < FCEU_FDSSides());
			this->side = side;
		}

		void select(View *view, const Input::Event &e)
		{
			if(side < FCEU_FDSSides())
			{
				FCEU_FDSSetDisk(side);
				viewStack.popAndShow();
			}
		}
	} setSide[4];

	TextMenuItem insertEject
	{
		"Eject",
		[](TextMenuItem &, const Input::Event &e)
		{
			if(FCEU_FDSInserted())
			{
				FCEU_FDSInsert();
				viewStack.popAndShow();
			}
		}
	};



	MenuItem *item[5] = {nullptr}; //sizeofArrayConst(setSide) + 2 not accepted by older GCC
public:
	FDSControlView(Base::Window &win): BaseMenuView("FDS Control", win) {}

	void init(bool highlightFirst)
	{
		uint i = 0;
		setSide[0].init("Set Disk 1 Side A", 0); item[i++] = &setSide[0];
		setSide[1].init("Set Disk 1 Side B", 1); item[i++] = &setSide[1];
		setSide[2].init("Set Disk 2 Side A", 2); item[i++] = &setSide[2];
		setSide[3].init("Set Disk 2 Side B", 3); item[i++] = &setSide[3];
		insertEject.init(FCEU_FDSInserted()); item[i++] = &insertEject;
		assert(i <= sizeofArray(item));
		BaseMenuView::init(item, i, highlightFirst);
	}
};

class SystemMenuView : public MenuView
{
private:
	struct FDSControlMenuItem : public TextMenuItem
	{
		constexpr FDSControlMenuItem() {}
		char label[sizeof("FDS Control (Disk 1:A)")] {0};
		void init()
		{
			strcpy(label, "");
			TextMenuItem::init(label);
		}

		void refreshActive()
		{
			active = isFDS;
			if(!isFDS)
				strcpy(label, "FDS Control");
			else if(!FCEU_FDSInserted())
				strcpy(label, "FDS Control (No Disk)");
			else
				sprintf(label, "FDS Control (Disk %d:%c)", (FCEU_FDSCurrentSide()>>1)+1, (FCEU_FDSCurrentSide() & 1)? 'B' : 'A');
			compile();
		}

		void select(View *view, const Input::Event &e)
		{
			if(EmuSystem::gameIsRunning() && isFDS)
			{
				auto &fdsMenu = *menuAllocator.allocNew<FDSControlView>(view->window());
				fdsMenu.init(!e.isPointer());
				viewStack.pushAndShow(fdsMenu, &menuAllocator);
			}
			else
				popup.post("Disk System not in use", 2);
		}
	} fdsControl;

	TextMenuItem cheats
	{
		"Cheats",
		[this](TextMenuItem &item, const Input::Event &e)
		{
			if(EmuSystem::gameIsRunning())
			{
				auto &cheatsMenu = *menuAllocator.allocNew<CheatsView>(window());
				cheatsMenu.init(!e.isPointer());
				viewStack.pushAndShow(cheatsMenu, &menuAllocator);
			}
		}
	};

public:
	SystemMenuView(Base::Window &win): MenuView(win) {}

	void onShow()
	{
		MenuView::onShow();
		cheats.active = EmuSystem::gameIsRunning();
		fdsControl.refreshActive();
	}

	void init(bool highlightFirst)
	{
		logMsg("init menu");
		uint items = 0;
		loadFileBrowserItems(item, items);
		fdsControl.init(); item[items++] = &fdsControl;
		cheats.init(); item[items++] = &cheats;
		loadStandardItems(item, items);
		assert(items <= sizeofArray(item));
		BaseMenuView::init(item, items, highlightFirst);
	}
};
