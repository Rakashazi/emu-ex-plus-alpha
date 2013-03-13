#pragma once
#include "OptionView.hh"
#include <libgen.h>

static bool isFDSBIOSExtension(const char *name);
static void setupNESInputPorts();
static void setupNESFourScore();

class SystemOptionView : public OptionView
{
private:

	BiosSelectMenu biosSelectMenu {&::fdsBiosPath, biosFsFilter};
	char fdsBiosPathStr[256] {0};
	TextMenuItem fdsBiosPath {"", TextMenuItem::SelectDelegate::create<template_mfunc(SystemOptionView, fdsBiosPathHandler)>(this)};

	template <size_t S>
	static void printBiosMenuEntryStr(char (&str)[S])
	{
		char basenameStr[S];
		strcpy(basenameStr, ::fdsBiosPath);
		string_printf(str, "Disk System BIOS: %s", strlen(::fdsBiosPath) ? basename(basenameStr) : "None set");
	}

	void biosPathUpdated()
	{
		logMsg("set fds bios %s", ::fdsBiosPath);
		printBiosMenuEntryStr(fdsBiosPathStr);
		fdsBiosPath.compile();
	}

	void fdsBiosPathHandler(TextMenuItem &, const Input::Event &e)
	{
		biosSelectMenu.init(!e.isPointer());
		biosSelectMenu.placeRect(Gfx::viewportRect());
		biosSelectMenu.biosChangeDel.bind<SystemOptionView, &SystemOptionView::biosPathUpdated>(this);
		modalView = &biosSelectMenu;
		Base::displayNeedsUpdate();
	}

	BoolMenuItem fourScore {"4-Player Adapter", BoolMenuItem::SelectDelegate::create<&fourScoreHandler>()};

	static void fourScoreHandler(BoolMenuItem &item, const Input::Event &e)
	{
		item.toggle();
		optionFourScore = item.on;
		setupNESFourScore();
	}

	MultiChoiceSelectMenuItem inputPorts {"Input Ports", MultiChoiceMenuItem::ValueDelegate::create<&inputPortsSet>()};

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

	static void inputPortsSet(MultiChoiceMenuItem &, int val)
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

public:
	constexpr SystemOptionView() { }

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

	TextMenuItem insertEject {"Eject", TextMenuItem::SelectDelegate::create<&insertEjectHandler>()};

	static void insertEjectHandler(TextMenuItem &, const Input::Event &e)
	{
		if(FCEU_FDSInserted())
		{
			FCEU_FDSInsert();
			viewStack.popAndShow();
		}
	}

	MenuItem *item[5] = {nullptr}; //sizeofArrayConst(setSide) + 2 not accepted by older GCC
public:
	constexpr FDSControlView(): BaseMenuView("FDS Control") { }

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

static FDSControlView fdsMenu;

class SystemMenuView : public MenuView
{
private:
	struct FDSControlMenuItem : public TextMenuItem
	{
		constexpr FDSControlMenuItem() { }
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
				fdsMenu.init(!e.isPointer());
				viewStack.pushAndShow(&fdsMenu);
			}
			else
				popup.post("Disk System not in use", 2);
		}
	} fdsControl;

	static void cheatsHandler(TextMenuItem &item, const Input::Event &e)
	{
		if(EmuSystem::gameIsRunning())
		{
			cheatsMenu.init(!e.isPointer());
			viewStack.pushAndShow(&cheatsMenu);
		}
	}

	TextMenuItem cheats {"Cheats"};

public:
	constexpr SystemMenuView() { }

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
		cheats.selectDelegate().bind<&cheatsHandler>();
		loadStandardItems(item, items);
		assert(items <= sizeofArray(item));
		BaseMenuView::init(item, items, highlightFirst);
	}
};
