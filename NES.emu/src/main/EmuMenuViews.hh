#pragma once
#include <emuframework/OptionView.hh>
#include <emuframework/MenuView.hh>
#include "EmuCheatViews.hh"

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
		[this](TextMenuItem &, View &, const Input::Event &e)
		{
			auto &biosSelectMenu = *new BiosSelectMenu{"Disk System BIOS", &::fdsBiosPath, biosFsFilter, window()};
			biosSelectMenu.init(!e.isPointer());
			biosSelectMenu.onBiosChange() =
				[this]()
				{
					logMsg("set fds bios %s", ::fdsBiosPath.data());
					printBiosMenuEntryStr(fdsBiosPathStr);
					fdsBiosPath.compile(projP);
				};
			viewStack.pushAndShow(biosSelectMenu);
		}
	};

	template <size_t S>
	static void printBiosMenuEntryStr(char (&str)[S])
	{
		FsSys::PathString basenameTemp;
		string_printf(str, "Disk System BIOS: %s", strlen(::fdsBiosPath.data()) ? string_basename(::fdsBiosPath, basenameTemp) : "None set");
	}

	BoolMenuItem fourScore
	{
		"4-Player Adapter",
		[this](BoolMenuItem &item, View &, const Input::Event &e)
		{
			item.toggle(*this);
			optionFourScore = item.on;
			setupNESFourScore();
		}
	};

	MultiChoiceSelectMenuItem inputPorts
	{
		"Input Ports",
		[](MultiChoiceMenuItem &, View &, int val)
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
		[](MultiChoiceMenuItem &, View &, int val)
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

class FDSControlView : public TableView
{
private:
	TextMenuItem setSide[4]
	{
		{
			"Set Disk 1 Side A",
			[](TextMenuItem &, View &view, const Input::Event &e)
			{
				FCEU_FDSSetDisk(0);
				view.popAndShow();
			}
		},
		{
			"Set Disk 1 Side B",
			[](TextMenuItem &, View &view, const Input::Event &e)
			{
				FCEU_FDSSetDisk(1);
				view.popAndShow();
			}
		},
		{
			"Set Disk 2 Side A",
			[](TextMenuItem &, View &view, const Input::Event &e)
			{
				FCEU_FDSSetDisk(2);
				view.popAndShow();
			}
		},
		{
			"Set Disk 2 Side B",
			[](TextMenuItem &, View &view, const Input::Event &e)
			{
				FCEU_FDSSetDisk(3);
				view.popAndShow();
			}
		}
	};

	TextMenuItem insertEject
	{
		"Eject",
		[](TextMenuItem &, View &, const Input::Event &e)
		{
			if(FCEU_FDSInserted())
			{
				FCEU_FDSInsert();
				viewStack.popAndShow();
			}
		}
	};

	MenuItem *item[sizeofArrayConst(setSide) + 1]{};

public:
	FDSControlView(Base::Window &win): TableView{"FDS Control", win} {}

	void init(bool highlightFirst)
	{
		uint i = 0;
		setSide[0].init(0 < FCEU_FDSSides()); item[i++] = &setSide[0];
		setSide[1].init(1 < FCEU_FDSSides()); item[i++] = &setSide[1];
		setSide[2].init(2 < FCEU_FDSSides()); item[i++] = &setSide[2];
		setSide[3].init(3 < FCEU_FDSSides()); item[i++] = &setSide[3];
		insertEject.init(FCEU_FDSInserted()); item[i++] = &insertEject;
		assert(i <= sizeofArray(item));
		TableView::init(item, i, highlightFirst);
	}
};

class SystemMenuView : public MenuView
{
private:
	char diskLabel[sizeof("FDS Control (Disk 1:A)")]{};

	TextMenuItem fdsControl
	{
		"",
		[this](TextMenuItem &item, View &, const Input::Event &e)
		{
			if(EmuSystem::gameIsRunning() && isFDS)
			{
				auto &fdsMenu = *new FDSControlView{window()};
				fdsMenu.init(!e.isPointer());
				pushAndShow(fdsMenu);
			}
			else
				popup.post("Disk System not in use", 2);
		}
	};

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

	void refreshFDSItem()
	{
		fdsControl.active = isFDS;
		if(!isFDS)
			strcpy(diskLabel, "FDS Control");
		else if(!FCEU_FDSInserted())
			strcpy(diskLabel, "FDS Control (No Disk)");
		else
			sprintf(diskLabel, "FDS Control (Disk %d:%c)", (FCEU_FDSCurrentSide()>>1)+1, (FCEU_FDSCurrentSide() & 1)? 'B' : 'A');
		fdsControl.t.setString(diskLabel);
		fdsControl.compile(projP);
	}

public:
	SystemMenuView(Base::Window &win): MenuView{win} {}

	void onShow()
	{
		MenuView::onShow();
		cheats.active = EmuSystem::gameIsRunning();
		refreshFDSItem();
	}

	void init(bool highlightFirst)
	{
		name_ = appViewTitle();
		uint items = 0;
		loadFileBrowserItems(item, items);
		fdsControl.init(); item[items++] = &fdsControl;
		cheats.init(); item[items++] = &cheats;
		loadStandardItems(item, items);
		assert(items <= sizeofArray(item));
		TableView::init(item, items, highlightFirst);
	}
};
